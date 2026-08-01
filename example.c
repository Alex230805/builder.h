#define BUILDER_IMP
#include "builder.h"


int main(int argc, char** argv){
	/* to use the auto rebuild, please define main() aith int argc, char** argv*/
	auto_rebuild("example.c", "example");
	
	printf("Testing build system\n");
	printf("Parsing command line arguments\n");
	if(argc > 1){
		for(int i=1;i<argc; i++){
			if(ARG_IS("test")){
				printf("received %s\n", argv[i]);
			}else{
				printf("parameter not recognised: %s\n", argv[i]);
			}
		}
	}

	if(MACOS){
		printf("Current execution environment: macos\n");
	}else if(LINUX){
		printf("Current execution environment: linux\n");
	}else{
		printf("Current execution environment: unknown\n");
	}

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
	path_append(p, f->contents_name[0]);
	printf("New path: %s\n", p->raw_path);
	printf("Exiting build system\n");
	
	path_destroy(p);
	p = NULL;
	// Destructor are needed if you want to deallocate memory from the allocator of build.h
	// Data structure that interact with builder.h and are intended to be managed by the user
	// are allocated with local_alloc() and then the pointer is freeze_ptr() to extend the 
	// lifetime. This default behaviour was introduced to avoit pointer invalidation of local 
	// objects like folder content, path tree names and so on, while working with complex 
	// build system configuration.
	// This object retain memory inside the allocator no matter the execution time or total
	// allocation amount.
	// To prevente an overflow it's suggested to avoid allocating too many objects of the same 
	// type and work with what already is present ( for example working with the same small pool of Cmd struct )
	// and/or call release_ptr() on the objects that are returned from builder.h functions like 
	// Folder and Path, which will flag them as usable and it allow the allocator to 
	// use the same address to allocate new information if needed.
	// 
	// Functions that return strings do not require a release_ptr(). e.g get_sha256();
	//
	
	cmd_list_destroy(&cmd_list);
	folder_destroy(f);
	printf("Testing custom search path\n");
	print_search_path();
	set_search_path("/custom/path");
	print_search_path();
	set_search_path("/another/search/path/that/will/be/used/to/search/binaries");
	print_search_path();
	return 0;
}

