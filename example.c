#define BUILDER_IMP
#include "builder.h"


int main(){
	printf("Testing build system\n");
	Cmd cmd[2] = {0};
	Cmd_List cmd_list = {0};
	cmd_set(cmd[0], "gcc", "-I./", "example.c", "-o", "out1");
	cmd_set(cmd[1], "gcc", "-I./", "example.c", "-o", "out2")
	cmd_list_append(&cmd_list,&cmd[0]);
	cmd_list_append(&cmd_list,&cmd[1]);

	wait_on_process_list(spawn_process_list(&cmd_list));

	printf("Exiting build system\n");
}
