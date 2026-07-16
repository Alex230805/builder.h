#define BUILDER_IMP
#include "builder.h"


int main(){
	/* WARNING: this depends on sha1 installed in your system */
	auto_rebuild("example.c", "example");
	printf("Testing build system\n");
	Cmd cmd[2] = {0};
	Cmd_List cmd_list = {0};
	cmd_set(cmd[0], "gcc", "-I./", "example.c", "-o", "out1");
	cmd_set(cmd[1], "gcc", "-I./", "example.c", "-o", "out2")
	cmd_list_append(&cmd_list,&cmd[0]);
	cmd_list_append(&cmd_list,&cmd[1]);

	wait_on_process_list(spawn_process_list(&cmd_list));
	printf("Testing sha generation for 'Test'\n");
	char* sha512 = get_sha512("Test");
	char* sha256 = get_sha256("Test");
	char* sha1 = get_sha1("Test");
	printf("sha512 of test: %s\n", sha512);
	printf("sha256 of test: %s\n", sha256);
	printf("sha1 of test: %s\n", sha1);

	printf("Testing folder ls content: \n");
	Folder* f = get_dir_content("./");
	for(size_t i=0;i<f->tracker; i++){
		printf("%s\n", f->contents_name[i]);
	}
	printf("Testing folder grep content: \n");
	f = grep_from_dir("./", "E");
	for(size_t i=0;i<f->tracker; i++){
		printf("%s\n", f->contents_name[i]);
	}

	printf("Custom path composition\n");
	char* current_p = get_current_path();
	Path* p = path_chop(current_p);
	printf("Og path: %s\n", p->raw_path);
	path_append_to(p, f->contents_name[0]);
	printf("New path: %s\n", p->raw_path);
	printf("Exiting build system\n");
}
