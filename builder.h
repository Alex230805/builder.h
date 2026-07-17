#ifndef BUILDER_H
#define BUILDER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>

#define INIT_P_RETURN 69420
#define DEFAULT_CMD_LIST_SIZE 4
#define DEFAULT_PATH_SIZE	512
#define LOCAL_SIZE 1024*64
#define HASH_FILE "./.builder_hash"
#define DEFAULT_FOLDER_SIZE 16

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

#define WIN32 false // not supported yet

static char path[DEFAULT_PATH_SIZE] = {0}; // main path, used to spawn processes from their location


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

// path is absolute by default, set flag 'not_abs' manually or use the function 'path_set_mode()'
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

static char* local_mem[LOCAL_SIZE] =  {0};
static size_t local_tracker =	0;
static size_t local_size =		LOCAL_SIZE;

static char static_mem[LOCAL_SIZE] = {0};
static size_t static_tracker = 0;
static size_t static_size = LOCAL_SIZE;

void cmd_set_imp(Cmd* cmd, char* list[]);
void* local_alloc(size_t size);
void local_free();
void* static_alloc(size_t size);

char* get_current_path();
void set_static_path(const char* static_path);
bool search_default_valid_path(char* executable);

Path* path_chop(char* path);
void path_set_mode(Path* p, bool p_type);
void path_append_to(Path* p, char* folder);

void path_render(char* path, Path* p);
void path_append(const char* dir);
char *path_compose(char* path, char* string);

void cmd_append(Cmd* cmd, char* string);
void cmd_list_append(Cmd_List* list, Cmd* cmd);

pid_t cmd_execute(Cmd* cmd);
pid_t* cmd_execute_list(Cmd_List* cmd);

Process* spawn_process(Cmd* cmd);
Process_View* spawn_process_list(Cmd_List* cmd);

void wait_on_process(Process* proc);
void wait_on_process_list(Process_View* procs);

static void capture_return(Process* process);

char* get_sha512(char* string);
char* get_sha256(char* string);
char* get_sha1(char* string);

char* get_sha512_from_file(char* file);
char* get_sha256_from_file(char* file);
char* get_sha1_from_file(char* file);

char* read_file(char* file);
void  write_file(char* file, char* content);

Folder* get_dir_content(char* path);
Folder* grep_from_dir(char* path, char* needle);


void auto_rebuild(char* src, char* output_name);

#ifdef BUILDER_IMP

void cmd_set_imp(Cmd* cmd, char* list[]){
	cmd->tracker = 0;
	int i=0;
	while(list[i] != NULL){
		cmd_append(cmd, list[i]);
		i+=1;	
	}
}

void* local_alloc(size_t size){
	if(local_tracker >= local_size) local_tracker = 0;
	local_mem[local_tracker] = (char*)malloc(size);
	void* ptr = local_mem[local_tracker];
	local_tracker += 1;
	return ptr;
}

void local_free(){
	for(size_t i=0;i<local_tracker; i++){
		free(local_mem[i]);
		local_mem[i] = NULL;
	}
	local_tracker = 0;
}

void* static_alloc(size_t size){
	if(static_tracker + size >= static_size) static_tracker = 0;
	void* ptr = &static_mem[static_tracker];
	static_tracker += size;	
	return ptr;
}

void cmd_append(Cmd* cmd, char* string){
	if(cmd->array == NULL){
		cmd->array = (char**)local_alloc(sizeof(char*)*32);
		cmd->size = 32;
		cmd->tracker = 0;
	}
	cmd->array[cmd->tracker] = string;
	cmd->tracker += 1;
	if(cmd->tracker >= cmd->size){
		char** old = cmd->array;
		cmd->array = (char**)local_alloc(sizeof(char*)*cmd->size*2);
		cmd->size *= 2;
		for(size_t i=0;i<cmd->tracker; i++){
			cmd->array[i] = old[i];
		}
	}
}

void set_static_path(const char* static_path){
	path[0] = '\0';
	memcpy(path, static_path, sizeof(char)*(strchr(static_path, '\0')+1-static_path));
}


char* get_current_path(){
	size_t size = 2048;
	FILE* fp = popen("echo $PWD", "r");
	char* p = (char*)local_alloc(sizeof(char)*size+1);
	fread(p, sizeof(char), size, fp);
	p[size] = '\0';
	*(strchr(p, '\n')) = '\0';
	pclose(fp);
	return p;
}

Path* path_chop(char* path){
	char* n = NULL;
	int i=0;
	char* cache = path;
	do{ n = strchr(cache, '/'); i+=1; cache  = n+1; }while(n != NULL);
	cache = path;
	Path* p = (Path*)local_alloc(sizeof(Path));
	p->depth = i;
	p->size = DEFAULT_PATH_SIZE;
	p->tree = (char**)local_alloc(sizeof(char*)*p->size);
	p->raw_path = NULL;
	bool end = false;
	size_t tracker = 0;
	for(i = 0;i<p->depth && !end; i++){
		n = strchr(cache, '/');
		if(n == NULL) {
			n = strchr(cache, '\0');
			end = true;
		}
		int len = n - cache;
		if(len > 0){
			p->tree[tracker] = (char*)malloc(sizeof(char)*len+1);	
			memcpy(p->tree[tracker], cache, sizeof(char)*len);
			p->tree[tracker][len] = '\0';
			tracker += 1;
		}
		cache = n+1;
	}
	p->depth = tracker;
	size_t render_size = 0;
	for(size_t i=0;i<p->depth; i++){
		render_size += strlen(p->tree[i]);
	}
	render_size*=2;
	p->raw_path = (char*)local_alloc(sizeof(char)*render_size);
	p->raw_path[0] = '\0';
	size_t s = 0;
	if(!p->not_abs){
		strcat(p->raw_path, "/");
		s += 1;
	}
	for(int i=0;i<p->depth; i++){
		strcat(p->raw_path, p->tree[i]);
		if(i+1 < p->depth){
			strcat(p->raw_path, "/");	
		}
	}
	return p;
}

void path_append_to(Path* p, char* folder){
	if(p == NULL || p->size < DEFAULT_PATH_SIZE){
		p->depth = 0;
		p->size = DEFAULT_PATH_SIZE;
		p->tree = (char**)local_alloc(sizeof(char*)*p->size);
	}
	if(p->depth+1 > p->size){
		char** old = p->tree;
		p->tree = (char**)local_alloc(sizeof(char*)*p->size*2);
		p->size *= 2;
		for(size_t i=0;i<p->depth; i++){
			p->tree[i] = old[i];
		}
	}
	p->tree[p->depth] = strdup(folder);
	p->depth += 1;
	size_t render_size = 0;
	for(size_t i=0;i<p->depth; i++){
		render_size += strlen(p->tree[i]);
	}
	render_size*=2;
	p->raw_path = (char*)local_alloc(sizeof(char)*render_size);
	p->raw_path[0] = '\0';
	size_t s = 0;
	if(!p->not_abs){
		strcat(p->raw_path, "/");
		s += 1;
	}
	for(int i=0;i<p->depth; i++){
		strcat(p->raw_path, p->tree[i]);
		if(i+1 < p->depth){
			strcat(p->raw_path, "/");	
		}
	}
}

void path_set_mode(Path* p, bool p_type){
	p->not_abs = p_type;
}

void path_render(char* path, Path* p){
	path[0] = '\0';
	for(int i=0;i<p->depth; i++){
		strcat(path, "/");
		strcat(path, p->tree[i]);
	}
}

void path_append(const char* dir){
	Path* p = path_chop(path);
	if(p->depth+1 >= p->size) return;
	p->tree[p->depth] = (char*)local_alloc(sizeof(char)*strlen(dir));
	memcpy(p->tree[p->depth], dir, strlen(dir));
	p->depth += 1;
	path_render(path, p);
}

char *path_compose(char* path, char* string){
	int size = strlen(path) + strlen(string);
	char* out = (char*)local_alloc((sizeof(char)*size)+2);
	strcpy(out, path);
	strcat(out, "/");
	strcat(out, string);
	return out;
}

void cmd_list_append(Cmd_List* list, Cmd* cmd){
	if(list->size == 0){
		list->array = (Cmd**)local_alloc(sizeof(Cmd*)*DEFAULT_CMD_LIST_SIZE);
		list->size = DEFAULT_CMD_LIST_SIZE;
		list->tracker = 0;
	}
	list->array[list->tracker] = cmd;
	list->tracker += 1;
	if(list->tracker >= list->size){
		Cmd** old_array = list->array;
		list->array = (Cmd**)local_alloc(sizeof(Cmd*)*list->size*2);
		list->size *= 2;
		for(int i=0;i<list->tracker; i++){
			list->array[i] = old_array[i];
		}
	}
}

bool search_default_valid_path(char* executable){
	char* buffer = NULL;
	size_t size = 4096;
	FILE* fp = popen("echo $PATH", "r");
	buffer = (char*)local_alloc((sizeof(char)*size)+1);

	fread(buffer, sizeof(char), size, fp);
	buffer[size] = '\0';
	*(strchr(buffer, '\n')) = '\0';

	char* c = strchr(buffer, ':');
	char* pos = (char*)local_alloc(sizeof(char)*DEFAULT_PATH_SIZE);

	bool end = false;
	while(!end){
		int size = c - buffer;
		memcpy(pos, buffer, size);
		pos[size] = '\0';
		strcat(pos, "/");
		strcat(pos, executable);
		if(!access(pos, F_OK)){
			memcpy(&path[0], buffer, size);
			end = true;
		}
		if(*c == '\0') break;
		buffer = c+1;
		c = strchr(buffer, ':');
		if(c == NULL){
			c = strchr(buffer, '\0');
		}
	}
	pos = NULL;
	pclose(fp);
	return end;
}

pid_t cmd_execute(Cmd* cmd){
	char* ex = path_compose(path, cmd->array[0]);
	if(access(ex, F_OK) != 0){
		printf("[WARNING]: path not provided for '%s', using defaults from $PATH\n", cmd->array[0]);
		if(!search_default_valid_path(cmd->array[0])){
			fprintf(stderr, "Unable to locate executable %s, please provide the search path using 'set_static_path', or 'get_current_path' for executable in the current location\n", cmd->array[0]);
			return 0;
		}
		ex = path_compose(path, cmd->array[0]);
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
	for(int i=0;i<cmd->tracker; i++){
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

	for(int i=0;i<cmd->tracker; i++){
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
		for(int i=0;i<procs->size;i++){
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
		fprintf(stderr, "Cannot open file: %s\n", strerror(errno));
		exit(errno);
	}
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* buffer = (char*)local_alloc(sizeof(char)*size+1);
	fread(buffer, sizeof(char), size, fp);
	buffer[size] = '\0';
	fclose(fp);
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


Folder* get_dir_content(char* path){
	Folder* f = (Folder*)local_alloc(sizeof(Folder));
	f->tracker = 0;
	f->size = DEFAULT_FOLDER_SIZE;
	f->contents_name = (char**)local_alloc(sizeof(char*)*f->size);

	DIR* dir = NULL;
	struct dirent *e = NULL;
	dir = opendir(path);
	if(dir == NULL) return NULL;
	while((e = readdir(dir)) != NULL){
		char* name = (char*)local_alloc(sizeof(char)*strlen(e->d_name)+1);
		strcpy(name, e->d_name);
		name[strlen(e->d_name)] = '\0';
		f->contents_name[f->tracker] = name;
		f->tracker += 1;
		if(f->tracker >= f->size){
			char** old = f->contents_name;
			f->contents_name = (char**)local_alloc(sizeof(char*)*f->size*2);
			f->size *= 2;
			for(size_t i=0;i<f->tracker;i++){
				f->contents_name[i] = old[i];
			}
		}
	}
	closedir(dir);
	return f;
}


Folder* grep_from_dir(char* path, char* needle){
	Folder* s = get_dir_content(path);
	char** buffer = (char**)local_alloc(sizeof(char*)*s->tracker);
	size_t nt = 0;
	for(size_t i=0;i<s->tracker; i++){
		if(strstr(s->contents_name[i], needle) != NULL){
			buffer[nt] = s->contents_name[i];
			nt += 1;
		}
	}
	s->contents_name = buffer;
	s->tracker = nt;
	return s;
}

void auto_rebuild(char* src_name, char* output_name){
	if(access(HASH_FILE, F_OK) != 0){
		char* sha = get_sha256_from_file(src_name);
		write_file(HASH_FILE, sha);
		return;
	}
	char* current = get_sha256_from_file(src_name);
	char* old	  = read_file(HASH_FILE);

	if(memcmp(old, current, strlen(old)) != 0){
		printf("[WARNING]: File differs, rebuilding system\n");
		Cmd cmd = {0};
		cmd_set(cmd, "gcc", src_name, "-o", output_name);
		wait_on_process(spawn_process(&cmd));
		char* sha = get_sha256_from_file(src_name);
		write_file(HASH_FILE, sha);

		set_static_path(get_current_path());
		cmd_set(cmd, output_name);
		wait_on_process(spawn_process(&cmd));
		exit(0);
	}
	return;
}

#endif
#endif // BUILDER_H
