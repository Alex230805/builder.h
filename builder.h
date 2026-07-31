#ifndef BUILDER_H
#define BUILDER_H


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>

#define HASH_FILE "./.builder_hash"
#define INIT_P_RETURN 133769
#define DEFAULT_FOLDER_SIZE	16
#define DEFAULT_CMD_LIST_SIZE 16
#define DEFAULT_CMD_SIZE 8

#define cmd_set(cmd, ...)\
		cmd_set_imp((&cmd), (char* []){__VA_ARGS__, NULL});

#ifdef __APPLE__
	#define MACOS true
#else 
	#define MACOS false
#endif

#ifdef __linux
	#define LINUX true
#else 
	#define LINUX false
#endif

#define WIN32 false // not supported yet, fuck microsoft 

#ifndef ENABLE_ALLOCATOR_LOG
	#define ENABLE_ALLOCATOR_LOG false
#endif

#define ARRAY_SIZE(buffer) (*((int*)(buffer-sizeof(uintptr_t))))

#define IS_ARGV_AT(w, i)\
	(strcmp(argv[(i)], (w)) ? false : true )

#define PRINT(prefix, content, ...)\
	fprintf(stdout, "["prefix"] "content"\n", __VA_ARGS__);

#define ERROR(prefix, content, ...)\
		fprintf(stdout, "[ERROR: "prefix"] "content"\n", __VA_ARGS__);


#define sync_run(cmd)						\
	do{										\
		wait_on_process(spawn_process(&(cmd)));\
	}while(0)								\

#define async_run(cmd)			\
	do{							\
		spawn_process(&(cmd));	\
	}while(0)					\

#define DEFAULT_ALLOCATOR_BUFFER_LENGTH 1024*64
#define COPY_BUFFER_SIZE 1024
#define DEFAULT_SEARCH_PATH_SIZE 512

typedef struct{
	char** array;
	size_t tracker;
	size_t size;
}Cmd;

typedef struct{
	Cmd** array;
	size_t tracker;
	size_t size;
}Cmd_List;

typedef struct{
	pid_t pid;
	int  ret_status; 
}Process;

typedef struct{
	Process** array;
	size_t size;
}Process_View;


typedef struct{
	char** tree;
	size_t depth;
	size_t size;
	char*  raw_path;
	bool not_abs;
}Path;

typedef struct{
	char** contents_name; 
	size_t size;
	size_t tracker;
}Folder; 


typedef struct{
	char memory[DEFAULT_ALLOCATOR_BUFFER_LENGTH];			// main memory pool tracker;
	size_t memory_tracker;

	void* memory_journal[DEFAULT_ALLOCATOR_BUFFER_LENGTH];  // memory pointers journal
	int memory_status[DEFAULT_ALLOCATOR_BUFFER_LENGTH];		// memory status indicator, mapped to tjournal
	int memory_size[DEFAULT_ALLOCATOR_BUFFER_LENGTH];		// memory status indicator, mapped to tjournal
	int memory_journal_tracker;
		

	char static_memory[DEFAULT_ALLOCATOR_BUFFER_LENGTH];   // static memory pool. Dedicated to threads
	size_t static_memory_tracker;
}Allocator;


static Allocator allocator = {0};

static char global_search_path[DEFAULT_SEARCH_PATH_SIZE] = {0};

#define ALLOCATOR_SIZE (sizeof(Allocator))

#define ALLOCATED 1
#define FREE 0
#define PERMANENT 2


void auto_rebuild(char* src_name, char* output_name);


void* local_alloc(int size);
void local_free(void* ptr);
void* static_alloc(int size);
void* freeze_ptr(void *ptr);
void* release_ptr(void* ptr);
char* local_strdup(char* str);


char* get_current_path();
void set_search_path(char* search_path);
bool search_valid_path(char* bin);


Path* path_chop(char* path);
void path_destroy(Path* p);
void path_render_raw(Path* p);
void path_append(Path* p, char* dir);
void path_set_mode(Path* p, bool p_type);


void cmd_set_imp(Cmd* cmd, char* list[]);
void cmd_append(Cmd* cmd, char* string);
void cmd_list_append(Cmd_List* list, Cmd* cmd);
void cmd_destroy(Cmd* cmd);
void cmd_list_destroy(Cmd_List* cmd_list);
pid_t cmd_execute(Cmd* cmd);
pid_t* cmd_execute_list(Cmd_List* cmd);
static void capture_return(Process* process);
Process* spawn_process(Cmd* cmd);
Process_View* spawn_process_list(Cmd_List* cmd);
void wait_on_process(Process* proc);
void wait_on_process_list(Process_View* procs);


char* get_sha512(char* string);
char* get_sha256(char* string);
char* get_sha1(char* string);


char* get_sha512_from_file(char* file);
char* get_sha256_from_file(char* file);
char* get_sha1_from_file(char* file);


char* read_file(char* file);
void  write_file(char* file, char* content);


void make_folder_if_not_exist(char* path);
char* shell_get_stdout(char* cmd, size_t size);


Folder* grep_from_dir(char* path, char* needle);
void folder_destroy(Folder* folder);
Folder* get_dir_content(char* path);


#ifdef BUILDER_IMP

void auto_rebuild(char* src_name, char* output_name){
	if(access(HASH_FILE, F_OK) != 0){
		char* sha = get_sha256_from_file(src_name);
		write_file(HASH_FILE, sha);
		local_free(sha);
		return;
	}
	char* current = get_sha256_from_file(src_name);
	char* old	  = read_file(HASH_FILE);

	if(memcmp(old, current, strlen(old)) != 0){
		PRINT("AUTOREBUILD", "Difference found inside '%s', rebuilding..", src_name);
		Cmd cmd = {0};
		cmd_set(cmd, "gcc", src_name, "-o", output_name);
		wait_on_process(spawn_process(&cmd));
		char* sha = get_sha256_from_file(src_name);
		write_file(HASH_FILE, sha);
		set_search_path(get_current_path());
		cmd_set(cmd, output_name);
		wait_on_process(spawn_process(&cmd));

		exit(0);
	}
	local_free(current);
	local_free(old);
	return;
}


void* local_alloc(int size){
	assert(size+sizeof(uintptr_t) < DEFAULT_ALLOCATOR_BUFFER_LENGTH);

	void* ptr = NULL;
	int position = allocator.memory_journal_tracker;

	for(int i=0;i<allocator.memory_journal_tracker; i++){
		if(allocator.memory_status[i] == FREE && allocator.memory_size[i] <= (int)(size+sizeof(uintptr_t))){
			ptr = allocator.memory_journal[i];
			if(ENABLE_ALLOCATOR_LOG) PRINT("local_alloc", "valid memory match usable", NULL);
			position = i;
			break;
		}
	}
	if(ptr == NULL){
		if(ENABLE_ALLOCATOR_LOG) PRINT("local_alloc", "grep new pointer from pool", NULL);
		bool data_hit = true;
		while(data_hit){
			data_hit = false;
			ptr = &allocator.memory[allocator.memory_tracker];
			int i;
			for(i=0;i<allocator.memory_journal_tracker; i++){
				if(allocator.memory_journal[i] == ptr && allocator.memory_status[i] == PERMANENT){
					data_hit = true;
					if(ENABLE_ALLOCATOR_LOG) PRINT("local_alloc", "data hit with another pointer which is flagged as freezed, searching for new pointer", NULL);
					break;
				}
			}
			if(data_hit){
				allocator.memory_tracker += allocator.memory_size[i];
			}
		}
		allocator.memory_tracker += size+sizeof(uintptr_t);
		allocator.memory_journal[position] = ptr;
		allocator.memory_journal_tracker += 1;
	}

	allocator.memory_status[position] = ALLOCATED;
	allocator.memory_size[position] = size+sizeof(uintptr_t);
	
	if(allocator.memory_tracker >= DEFAULT_ALLOCATOR_BUFFER_LENGTH) allocator.memory_tracker = 0;
	if(allocator.memory_journal_tracker >= DEFAULT_ALLOCATOR_BUFFER_LENGTH) allocator.memory_journal_tracker = 0;

	assert(ptr != NULL);
	if(ENABLE_ALLOCATOR_LOG) PRINT("local_alloc", "Allocating %p", ptr);
	memset(ptr, 0, size+sizeof(uintptr_t));
	ptr += sizeof(uintptr_t);
	ARRAY_SIZE(ptr) = size;
	return ptr;
}


void local_free(void* ptr){
	if(ENABLE_ALLOCATOR_LOG){
		PRINT("local_free", "free ptr %p", ptr);
	}
	for(int i=0;i<allocator.memory_journal_tracker; i++){
		if(ptr-sizeof(uintptr_t) == allocator.memory_journal[allocator.memory_journal_tracker] && allocator.memory_status[i] == ALLOCATED){
			if(ENABLE_ALLOCATOR_LOG) PRINT("local_free", "pointer match found, flag it to free now", NULL);
			allocator.memory_status[i] = FREE;
			break;
		}
	}
	return;
}

void* freeze_ptr(void *ptr){
	for(int i=0;i<allocator.memory_journal_tracker; i++){
		if(allocator.memory_journal[i] == ptr-sizeof(uintptr_t) && allocator.memory_status[i] == ALLOCATED){
			if(ENABLE_ALLOCATOR_LOG) PRINT("freeze_ptr", "allocated pointer %p is now freezed", ptr);
			allocator.memory_status[i] = PERMANENT;
			break;
		}
	}
	return ptr;
}

void* release_ptr(void* ptr){
	for(int i=0;i<allocator.memory_journal_tracker; i++){
		if(allocator.memory_journal[i] == ptr-sizeof(uintptr_t) && allocator.memory_status[i] == PERMANENT){
			if(ENABLE_ALLOCATOR_LOG) PRINT("release_ptr", "allocated pointer %p is now released, it can be deallocated now", ptr);
			allocator.memory_status[i] = ALLOCATED;
			break;
		}
	}
	return ptr;
}

char* local_strdup(char* str){
	if(str == NULL) return NULL;
	char* dest = (char*)local_alloc(sizeof(char)*strlen(str)+1);
	strcpy(dest, str);
	return dest;
}

void* static_alloc(int size){
	assert(size < DEFAULT_ALLOCATOR_BUFFER_LENGTH);
	void* ptr = &allocator.static_memory[allocator.static_memory_tracker];
	allocator.static_memory_tracker += size+sizeof(uintptr_t);
	if(allocator.static_memory_tracker >= DEFAULT_ALLOCATOR_BUFFER_LENGTH) allocator.static_memory_tracker=0;
	memset(ptr, 0, size+sizeof(uintptr_t));
	ptr += sizeof(uintptr_t);
	return ptr;
}

char* get_current_path(){
	size_t size = COPY_BUFFER_SIZE;
	FILE* fp = popen("echo $PWD", "r");
	char* p = (char*)local_alloc(sizeof(char)*size+1);
	fread(p, sizeof(char), size, fp);
	p[size] = '\0';
	char* nl = strchr(p, '\n');
	if(nl != NULL) *nl = '\0';
	pclose(fp);
	return freeze_ptr(p);
}


void set_search_path(char* search_path){
	if(search_path == NULL){
		search_path[0] = '\0';
		return;
	}
	strcpy(global_search_path, search_path);
}


bool search_valid_path(char* bin){
	char* buffer = NULL;
	size_t size = 4096;
	FILE* fp = popen("echo $PATH", "r");
	buffer = (char*)local_alloc((sizeof(char)*size)+1);
	fread(buffer, sizeof(char), size, fp);
	buffer[size] = '\0';
	if(strchr(buffer, '\n')) *(strchr(buffer, '\n')) = '\0';
	
	char* c = strchr(buffer, ':');
	char* pos = (char*)local_alloc(sizeof(char)*DEFAULT_SEARCH_PATH_SIZE);

	bool end = false;
	while(!end){
		int size = c - buffer;
		memcpy(pos, buffer, size);
		pos[size] = '\0';
		strcat(pos, "/");
		strcat(pos, bin);
		if(!access(pos, F_OK)){
			memcpy(global_search_path, buffer, size);
			global_search_path[size] = '\0';
			end = true;
		}
		if(*c == '\0') break;
		buffer = c+1;
		c = strchr(buffer, ':');
		if(c == NULL){
			c = strchr(buffer, '\0');
		}
	}
	local_free(pos);
	local_free(buffer);
	pclose(fp);
	return end;
}

void path_render_raw(Path* p){
	size_t render_size = 0;
	for(size_t i=0;i<p->depth; i++){
		render_size += strlen(p->tree[i]);
	}
	render_size*=2;
	p->raw_path = (char*)freeze_ptr(local_alloc(sizeof(char)*render_size));
	p->raw_path[0] = '\0';
	if(!p->not_abs){
		strcat(p->raw_path, "/");
	}
	for(size_t i=0;i<p->depth; i++){
		strcat(p->raw_path, p->tree[i]);
		if(i+1 < p->depth){
			strcat(p->raw_path, "/");	
		}
	}
	return;
}

Path* path_chop(char* path){
	char* n = NULL;
	size_t i=0;
	char* cache = path;
	do{ n = strchr(cache, '/'); i+=1; cache  = n+1; }while(n != NULL);
	cache = path;
	Path* p = (Path*)freeze_ptr(local_alloc(sizeof(Path)));
	p->depth = i;
	p->size = (i*2 > DEFAULT_SEARCH_PATH_SIZE) ? i*2 : DEFAULT_SEARCH_PATH_SIZE;
	p->tree = (char**)freeze_ptr(local_alloc(sizeof(char*)*p->size));
	p->raw_path = NULL;
	bool end = false;
	size_t tracker = 0;
	for(size_t i = 0;i<p->depth && !end; i++){
		n = strchr(cache, '/');
		if(n == NULL) {
			n = strchr(cache, '\0');
			end = true;
		}
		int len = n - cache;
		if(len > 0){
			p->tree[tracker] = (char*)freeze_ptr(local_alloc(sizeof(char)*len+1));
			memcpy(p->tree[tracker], cache, sizeof(char)*len);
			p->tree[tracker][len] = '\0';
			tracker += 1;
		}
		cache = n+1;
	}
	p->depth = tracker;
	path_render_raw(p);
	return p;
}

void path_destroy(Path* p){
	for(size_t i=0;i<p->size; i++){
		if(p->tree[i] != NULL){
			local_free(release_ptr(p->tree[i]));
		}
	}
	local_free(release_ptr(p->raw_path));
	local_free(release_ptr(p->tree));
	//local_free(release_ptr(p));
	p->tree = NULL;
	p->raw_path = NULL;
	p->depth = 0;
}



void path_append(Path* p, char* dir){
	if(p->depth+1 >= p->size){
		char** old = p->tree;
		p->tree = (char**)freeze_ptr(local_alloc(sizeof(char*)*p->size*2));
		p->size *= 2;
		for(size_t i=0;i<p->depth; i++){
			p->tree[i] = old[i];
		}
		local_free(release_ptr(old));
	}
	p->tree[p->depth] = local_strdup(dir);
	p->depth += 1;
	local_free(release_ptr(p->raw_path));
	p->raw_path = NULL;
	path_render_raw(p);
	return;
}

void path_set_mode(Path* p, bool p_type){
	p->not_abs = p_type;
}


void cmd_set_imp(Cmd* cmd, char* list[]){
	if(cmd->tracker > 0) cmd_destroy(cmd);
	cmd->tracker = 0;
	int i=0;
	while(list[i] != NULL){
		cmd_append(cmd, list[i]);
		i+=1;
	}
}

void cmd_append(Cmd* cmd, char* string){
	if(cmd->array == NULL){
		cmd->array = (char**)freeze_ptr(local_alloc(sizeof(char*)*DEFAULT_CMD_SIZE));
		cmd->size = DEFAULT_CMD_SIZE;
		cmd->tracker = 0;
	}
	cmd->array[cmd->tracker] = freeze_ptr(local_strdup(string));
	cmd->tracker += 1;
	if(cmd->tracker >= cmd->size){
		char** old = cmd->array;
		cmd->array = (char**)freeze_ptr(local_alloc(sizeof(char*)*cmd->size*2));
		cmd->size *= 2;
		for(size_t i=0;i<cmd->tracker; i++){
			cmd->array[i] = old[i];
		}
		local_free(release_ptr(old));
	}
}

void cmd_list_append(Cmd_List* list, Cmd* cmd){
	if(list->size == 0){
		list->array = (Cmd**)freeze_ptr(local_alloc(sizeof(Cmd*)*DEFAULT_CMD_LIST_SIZE));
		list->size = DEFAULT_CMD_LIST_SIZE;
		list->tracker = 0;
	}
	list->array[list->tracker] = cmd;
	list->tracker += 1;
	if(list->tracker >= list->size){
		Cmd** old_array = list->array;
		list->array = (Cmd**)freeze_ptr(local_alloc(sizeof(Cmd*)*list->size*2));
		list->size *= 2;
		for(size_t i=0;i<list->tracker; i++){
			list->array[i] = old_array[i];
		}
		local_free(release_ptr(old_array));
	}
}


void cmd_destroy(Cmd* cmd){
	for(size_t i=0;i<cmd->size; i++){
		if(cmd->array[i] != NULL){
			local_free(release_ptr(cmd->array[i]));
		}
	}
	local_free(release_ptr(cmd->array));
	cmd->array = NULL;
	cmd->tracker = 0;
	cmd->size = 0;
}

void cmd_list_destroy(Cmd_List* cmd_list){
	for(size_t i=0;i<cmd_list->size; i++){
		if(cmd_list->array[i] != NULL){
			cmd_destroy(cmd_list->array[i]);
		}
	}
	local_free(release_ptr(cmd_list->array));
	cmd_list->array = NULL;
	cmd_list->tracker = 0;
	cmd_list->size = 0;
}

pid_t cmd_execute(Cmd* cmd){
	Path* exp = path_chop(global_search_path);
	path_append(exp, cmd->array[0]);
	char* ex = exp->raw_path;
	if(access(ex, F_OK) != 0){
		PRINT("WARNING", "Path not provided for '%s', searching from system's default", cmd->array[0]);
		if(!search_valid_path(cmd->array[0])){
			ERROR("EXECUTABLE LOCATION", "Unable to locate executable '%s', please provide the search path using 'set_search_path' and 'get_current_path' for executable in the current location", cmd->array[0]);
			return 0;
		}
		path_destroy(exp);
		exp = path_chop(global_search_path);
		path_append(exp, cmd->array[0]);
		ex = exp->raw_path;
	}

	printf("[CMD]: [");
	for(size_t i=0;i<cmd->tracker; i++){
		if(i==0){
			printf("%s, ", ex);
		}else{
			printf("%s, ", cmd->array[i]);
		}
	}
	printf("NULL]\n");
	cmd_append(cmd, NULL);
	pid_t pid = fork();
	if(pid < 0){
		abort();
	}
	if(pid > 0){
		path_destroy(exp);
		return pid;
	}else{
		if(execv(ex, cmd->array) < 0){
			fprintf(stderr, "Unable to spawn process: %s\n", strerror(errno));
			abort();
		}
	}
	return 0;
}

pid_t* cmd_execute_list(Cmd_List* cmd){
	pid_t* pid = (pid_t*)local_alloc(sizeof(pid_t)*cmd->tracker);
	for(size_t i=0;i<cmd->tracker; i++){
		pid[i] = cmd_execute(cmd->array[i]);
	}
	return pid;
}

static void capture_return(Process* process){
	static int loc_ret = INIT_P_RETURN;
	waitpid(process->pid, &loc_ret, 0);
	process->ret_status = WEXITSTATUS(loc_ret);
}

Process* spawn_process(Cmd* cmd){
	pthread_t* monitor = (pthread_t*)static_alloc(sizeof(pthread_t));
	Process* proc = (Process*)static_alloc(sizeof(Process));
	pid_t p = cmd_execute(cmd);
	if(p > 0){
		proc->pid = p;
		proc->ret_status = INIT_P_RETURN;
		if(pthread_create(monitor, NULL, (void*)&capture_return, proc)){
			fprintf(stderr, "Unable to create thread: %s\n", strerror(errno));
		}
	}
	return proc;
}

Process_View* spawn_process_list(Cmd_List* cmd){
	Process_View* procs = (Process_View*)static_alloc(sizeof(Process_View));
	procs->array = (Process**)static_alloc(sizeof(Process*)*cmd->tracker);
	procs->size = cmd->tracker;

	for(size_t i=0;i<cmd->tracker; i++){
		procs->array[i] = spawn_process(cmd->array[i]);
	}
	return procs;
}

void wait_on_process(Process* proc){
	while(true){
		if(proc->ret_status != INIT_P_RETURN){
			break;
		}
	}
	return;
}

void wait_on_process_list(Process_View* procs){
	bool exit = false;
	while(!exit){
		exit = true;
		sleep(1);
		for(size_t i=0;i<procs->size;i++){
			if(procs->array[i]->ret_status == INIT_P_RETURN){
				exit = false;
			}
		}
	}
	return;
}

char* get_sha512(char* string){
#ifdef _WIN32 
	return NULL; // not implemented yet
#endif
	char stdin_buffer[1024];
#ifdef __APPLE__
	sprintf(stdin_buffer, "sha512 -s ");
	strcat(stdin_buffer, string);
#elif __linux__
	sprintf(stdin_buffer, "echo \"");
	strcat(stdin_buffer, string);
	strcat(stdin_buffer, "\" | sha512sum");
#endif
	FILE* s = popen(stdin_buffer, "r");
	if(s == NULL) return NULL;
	char* hash = (char*)local_alloc(sizeof(char)*1024);
	fread(hash, sizeof(char), 1024, s);
#ifdef __linux 
	*(strchr(hash, ' ')) = '\0';
#else
	*(strchr(hash, '\n')) = '\0';
#endif
	pclose(s);
	return hash;
}

char* get_sha512_from_file(char* file){
#ifdef _WIN32 
	return NULL; // not implemented yet
#endif
	char stdin_buffer[1024];
#ifdef __APPLE__
	sprintf(stdin_buffer, "sha512 --quiet ");
#elif __linux__
	sprintf(stdin_buffer, "sha512sum ");
#endif
	strcat(stdin_buffer, file);
	FILE* s = popen(stdin_buffer, "r");
	if(s == NULL) return NULL;
	char* hash = (char*)local_alloc(sizeof(char)*1024);
	fread(hash, sizeof(char), 1024, s);
#ifdef __linux 
	*(strchr(hash, ' ')) = '\0';
#else
	*(strchr(hash, '\n')) = '\0';
#endif
	pclose(s);
	return hash;
}

char* get_sha256(char* string){
#ifdef _WIN32 
	return NULL; // not implemented yet
#endif
	char stdin_buffer[1024];
#ifdef __APPLE__
	sprintf(stdin_buffer, "sha256 -s ");
	strcat(stdin_buffer, string);
#elif __linux__
	sprintf(stdin_buffer, "echo \"");
	strcat(stdin_buffer, string);
	strcat(stdin_buffer, "\" | sha256sum");
#endif
	FILE* s = popen(stdin_buffer, "r");
	if(s == NULL) return NULL;
	char* hash = (char*)local_alloc(sizeof(char)*1024);
	fread(hash, sizeof(char), 1024, s);
#ifdef __linux 
	*(strchr(hash, ' ')) = '\0';
#else
	*(strchr(hash, '\n')) = '\0';
#endif
	pclose(s);
	return hash;
}

char* get_sha256_from_file(char* file){
#ifdef _WIN32 
	return NULL; // not implemented yet
#endif
	char stdin_buffer[1024];
#ifdef __APPLE__
	sprintf(stdin_buffer, "sha256 --quiet ");
#elif __linux__
	sprintf(stdin_buffer, "sha256sum ");
#endif
	strcat(stdin_buffer, file);
	FILE* s = popen(stdin_buffer, "r");
	if(s == NULL) return NULL;
	char* hash = (char*)local_alloc(sizeof(char)*1024);
	fread(hash, sizeof(char), 1024, s);
#ifdef __linux 
	*(strchr(hash, ' ')) = '\0';
#else
	*(strchr(hash, '\n')) = '\0';
#endif
	pclose(s);
	return hash;
}

char* get_sha1(char* string){
#ifdef _WIN32 
	return NULL; // not implemented yet
#endif
	char stdin_buffer[1024];
#ifdef __APPLE__
	sprintf(stdin_buffer, "sha1 -s ");
	strcat(stdin_buffer, string);
#elif __linux__
	sprintf(stdin_buffer, "echo \"");
	strcat(stdin_buffer, string);
	strcat(stdin_buffer, "\" | sha1sum");
#endif
	FILE* s = popen(stdin_buffer, "r");
	if(s == NULL) return NULL;
	char* hash = (char*)local_alloc(sizeof(char)*1024);
	fread(hash, sizeof(char), 1024, s);
#ifdef __linux 
	*(strchr(hash, ' ')) = '\0';
#else
	*(strchr(hash, '\n')) = '\0';
#endif
	pclose(s);
	return hash;
}

char* get_sha1_from_file(char* file){
#ifdef _WIN32
	return NULL; // not implemented yet
#endif 
	char stdin_buffer[1024];
#ifdef __APPLE__
	sprintf(stdin_buffer, "sha1 --quiet ");
#elif __linux__
	sprintf(stdin_buffer, "sha1sum ");
#endif
	strcat(stdin_buffer, file);
	FILE* s = popen(stdin_buffer, "r");
	if(s == NULL) return NULL;
	char* hash = (char*)local_alloc(sizeof(char)*1024);
	fread(hash, sizeof(char), 1024, s);
#ifdef __linux 
	*(strchr(hash, ' ')) = '\0';
#else
	*(strchr(hash, '\n')) = '\0';
#endif
	pclose(s);
	return hash;
}

char* read_file(char* file){
	FILE* fp = fopen(file, "r");
	if(!fp){
		ERROR("READ FILE", "Cannot open %s: %s", file, strerror(errno));
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* buffer = (char*)local_alloc(sizeof(char)*size+1);
	fread(buffer, sizeof(char), size, fp);
	buffer[size] = '\0';
	fclose(fp);
	ARRAY_SIZE(buffer) = size;
	return buffer;
}

void  write_file(char* file, char* content){
	FILE* fp = fopen(file, "w");
	if(!fp){
		fprintf(stderr, "Cannot open file: %s\n", strerror(errno));
		exit(errno);
	}
	fwrite(content, sizeof(char), strlen(content), fp);
	fclose(fp);
}

char* shell_get_stdout(char* cmd, size_t size){
	FILE* fp = popen(cmd, "r");
	char* p = (char*)local_alloc(sizeof(char)*size+1);
	fread(p, sizeof(char), size, fp);
	p[size] = '\0';
	if(strchr(p, '\n')) *(strchr(p, '\n')) = '\0';
	pclose(fp);
	return p;
}

void make_folder_if_not_exist(char* path){
	DIR* dir = opendir(path);
	if(errno == ENOENT){
		Cmd cmd = {0};
		cmd_set(cmd, "mkdir", path);
		wait_on_process(spawn_process(&cmd));
		cmd_destroy(&cmd);
		return;
	}else if(dir){
		closedir(dir);
	}
}

Folder* grep_from_dir(char* path, char* needle){
	Folder* s = get_dir_content(path);
	char** buffer = (char**)freeze_ptr(local_alloc(sizeof(char*)*s->tracker));
	size_t nt = 0;
	for(size_t i=0;i<s->tracker; i++){
		if(strstr(s->contents_name[i], needle) != NULL){
			buffer[nt] = s->contents_name[i];
			nt += 1;
		}else{
			local_free(release_ptr(buffer[nt]));
			buffer[nt] = NULL;
		}
	}
	local_free(release_ptr(s->contents_name));
	s->contents_name = buffer;
	s->size = s->tracker;
	s->tracker = nt;
	return s;
}


void folder_destroy(Folder* folder){
	for(size_t i=0; i < folder->size; i++){
		if(folder->contents_name[i] != NULL){
			local_free(release_ptr(folder->contents_name[i]));
		}
	}
	local_free(release_ptr(folder->contents_name));
	local_free(release_ptr(folder));
}

Folder* get_dir_content(char* path){
	Folder* f = (Folder*)freeze_ptr(local_alloc(sizeof(Folder)));
	f->tracker = 0;
	f->size = DEFAULT_FOLDER_SIZE;
	f->contents_name = (char**)freeze_ptr(local_alloc(sizeof(char*)*f->size));

	DIR* dir = NULL;
	struct dirent *e = NULL;
	dir = opendir(path);
	if(dir == NULL) return NULL;

	while((e = readdir(dir)) != NULL){
		char* name = (char*)freeze_ptr(local_alloc(sizeof(char)*strlen(e->d_name)));
		strcpy(name, e->d_name);
		f->contents_name[f->tracker] = name;
		f->tracker += 1;
		if(f->tracker >= f->size){
			char** old = f->contents_name;
			f->contents_name = (char**)freeze_ptr(local_alloc(sizeof(char*)*f->size*2));
			f->size *= 2;
			for(size_t i=0;i<f->tracker;i++){
				f->contents_name[i] = old[i];
			}
			local_free(release_ptr(old));
		}
	}
	closedir(dir);
	return f;
}


#endif
#endif // BUILDER_H
