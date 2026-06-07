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

#define INIT_P_RETURN 69420
#define DEFAULT_CMD_LIST_SIZE 4
#define DEFAULT_PATH_SIZE	512
#define LOCAL_SIZE 1024*16

#define cmd_set(cmd, ...)\
		cmd_set_imp((&cmd), (char* []){__VA_ARGS__, NULL});


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

typedef struct{
	char** tree;
	size_t depth;
	size_t size;
}Path;


static char local_mem[LOCAL_SIZE] =  {0};
static size_t local_tracker =	0;
static size_t local_size =		LOCAL_SIZE;

void cmd_set_imp(Cmd* cmd, char* list[]);
void* local_alloc(size_t size);
void local_alloc_rewind(size_t size);

char* get_current_path();
void set_static_path(const char* static_path);
bool search_default_valid_path(char* executable);

Path* path_chop(char* path);
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
	if(size+local_tracker >= local_size) local_tracker = 0;
	void* ptr = &local_mem[local_tracker];
	local_tracker += size;
	if(local_tracker >= local_size) local_tracker = 0;
	return ptr;
}

void local_alloc_rewind(size_t size){
	if((int)local_tracker - (int)size < 0) {
		local_tracker = 0;
		return;
	}
	local_tracker -= size;
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
	strcpy(path, static_path);
}

char* get_current_path(){
	FILE* fp = popen("echo $PWD", "r");
	fseek(fp,0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* p = (char*)local_alloc(sizeof(char)*size+1);
	fread(p, sizeof(char), size, fp);
	p[size] = '\0';
	*(strchr(p, '\n')) = '\0';
	return p;
}

Path* path_chop(char* path){
	char* n = NULL;
	int i=0;
	char* cache = path;
	do{ n = strchr(path, '/'); i+=1; path  = n+1; }while(n != NULL);
	Path* p = (Path*)local_alloc(sizeof(Path));
	p->depth = i;
	p->size = DEFAULT_PATH_SIZE;
	p->tree = (char**)local_alloc(sizeof(char*)*p->size);
	int tracker = 0;
	for(i = 0;i<p->depth; i++){
		n = strchr(cache, '/');
		int len = n - cache;
		if(len > 1){
			p->tree[tracker] = (char*)local_alloc(sizeof(char)*len);	
			memcpy(p->tree[tracker], cache, sizeof(char)*len);
			tracker += 1;
		}
		cache = n+1;
	}
	p->depth = tracker;
	return p;
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
	FILE* fp = popen("echo $PATH", "r");
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buffer = (char*)local_alloc((sizeof(char)*size)+1);

	fread(buffer, sizeof(char), size, fp);
	buffer[size] = '\0';
	*(strchr(buffer, '\n')) = '\0';
	
	char* c = strchr(buffer, ':');
	char* pos = (char*)malloc(sizeof(char)*DEFAULT_PATH_SIZE);

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

	local_alloc_rewind((sizeof(char)*size)+1);
	free(pos);
	pos = NULL;
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
	pthread_t* monitor = (pthread_t*)local_alloc(sizeof(pthread_t));
	Process* proc = (Process*)local_alloc(sizeof(Process));
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
	Process_View* procs = (Process_View*)local_alloc(sizeof(Process_View));
	procs->array = (Process**)local_alloc(sizeof(Process*)*cmd->tracker);
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

#endif


#endif // BUILDER_H
