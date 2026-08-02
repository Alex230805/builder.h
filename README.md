# builder.h 

builder.h is a single header library made to create build scripts directly in C. It can handle async and sync building mode, all by using simple command line struct to compose different commands. <br>
It can act as a process spawner directly implemented in C. <br>


## How to build and use builder.h

builder.h is a header only library, it include both the declaration and implementation, and if you want to create a build script you must declare **BUILDER_SCRIPT** before including *builder.h*: <br>

```C

#define BUILDER_SCRIPT
#include "builder.h"

int main(){
	auto_rebuild("builder.c", "builder");
    // code here
}

```


By creating a separate file, the compilation process for a build script comes down to a single compiler line:  

```C

cc builder.c -o builder // if we assume the builder file is called builder.c
./builder             // start builder script 

```

<br>

It's suggested to use builder.h in a standalone file implementation ( for example in a file named *builder.c* ), after that to begin with the compilation the system must be bootstrapped at leat once ( if you don't use "auto_rebuild()" in your script you must bootstrap it each time you made modifications ). <br>

## Difference between "BUILDER_SCRIPT" and "BUILDER_IMP"

Builer.h include it's own allocator to perform internal processing and organise memory in a clean way. Creating a build script with "#define BUILDER_SCRIPT" will introduce a custom entry point which perform the allocator initialization process and then will call the builder entry point to start your build script. This technology is present to simplify the writing process of the build script, but it can lead to problems if the "builder.h" and it's implementation is included in files different than builder.c. <br>
<br>
To simplify, "#define BUILDER_SCRIPT" will automatically define "BUILDER_IMP" in itself, but if you want to use the "builder.h" with the implementation you can just use "#define BUILDER_IMP" which will not introduce the automatic initialization process of 
the allocator.<br>

#### Working without allocator

Builder.h uses the internal allocator only if the builder is initialized correcly, which imply that if the allocator is not initialized it will switch to plain malloc and free operation. Defining "BUILDER_IMP" before including "builder.h" will not initialize the allocator, and each functions that internally call to the allocator functions will require pointers outside the monitored memory scope and so destructor for data-structure will stop working while the memory management is redirected to the user who's working with builder.h functions. <br>
<br>
**Every functions inside builder.h is based on the internal allocator**, so every object internally **will allocate heap space if no allocator is initialized.** The custom "main()", which is only included with "#define BUILDER_SCRIPT" will automatically perform the allocator initialization for you, which is perfect for complex build scripts or applications that require a perfect memory management. <br>
**If you dont want to make builder.h fallback to plain malloc()/free() calls, you can manually call the allocator initialization functions:**

```C

// initialize the allocator
void init_alloc();

// close the allocator 
void close_alloc();

```

Which is the same thing that the custom main() entry point does when a build script is defined with "#define BUILDER_SCRIPT"
<br>

NOTE: About destructor, if compiled with "#define BUILDER_IMP", are deactivated by default due to the inability to identify correctly the allocated pointer from internal functions. This will cause memory segmentation if not handled correctly and it's suggested to external system to enable the internal allocator manually, or correctly handle the free for each allocated pointer with *local_free()*.<br><br>

> [!WARNING]
> Objects allocated with local_alloc() MUST be deallocated with local_free(). Each structure or buffer allocated from builder.h uses a **shadow data** trick to store the allocated side **before** the returned pointer with an offset of **sizeof(uintptr_t)**. So the 
> internal local_free() will fallback to free() with a call that looks like:
>
>   **free(ptr-sizeof(uintptr_t))**.
>
> **Any object, even if the allocation has been disabled, must be allocated and/or deallocated with local_alloc() and local_free()**. If you don't need the internal allocator and/or you are building your application with "BUILDER_IMP" enabled, you should manage memory ( *if you need* )
> only with local_alloc() and local_free().<br>
> This, of course, for the data used internally for builder.h.

#### Allocator and Destructor

Internal functions allocate on the internal allocator and then mark the pointer as **freeze** to tell the allocator to not use the same memory zone if the allocator buffer is full and need to allocate new datas. Every functions inside builder.h make use of the 
*freeze_ptr()* to accomodate this and return object to the build script that have a permanent lifetime, at least untill the pointer is specifically **released** with *release_ptr()*.<br>
This is exacly why there are destructor inside builder.h which release the pointer and free the memory location for you in order to maintain a good memory management inside the build script. <br>
In general is not something that you need to worry about since the allocator pool is big, but if your build system is large and perform cyclical operations that continue to require objects from builder.h then you must pay attention to memory management and object lifetimes.<br>
<br>
It's suggested to not allocate too many object of the same type and reuse them to manage the memory. For example "cmd_set" when called can check if the cmd if full before inserting new datas, and if it is then the destructor for cmd is called ( *cmd_destroy()* ) and then a new space is allocated and freezed for then be filled with the arguments provided. Same principle can be applied to Path. <br>
<br>
If inside your builder.c script you want to allocate memory manually then you can do that with *local_alloc()* and *local_free()*, and for manage their lifetimes you can call *freeze_ptr()* and *release_ptr()* like in this example: 


```C

    // allocate and set permanent lifetime for buffer of size 64
    char* buffer = freeze_ptr(local_alloc(sizeof(char)*64));   

    // release the pointer to then dealloc the buffer
    local_free(release_ptr(buffer));

```


## Why such system? 

With complex projects and rising complexity having a build system is almost mandatory, some developers use bash scripts, some use make files, with the only goal of **organise and control the build operation/s**. But having a build system has the disadvantage of introducing yet another dependency, which is not a bad thing per se, but it forces the client who build your project to have a **required** program in order to even begin with the compilation.

Builder.h is a library written in C that offer a toolkit to easily build projects, to execute code during compilation, while using a fully working programming language. With that **in order to build your project you just need a C compiler, that's it**. <br> 
If you script your build system correctly ( by using auto_rebuild() on top of your build script ) you just need to bootstrap it once, then you can just build your system as easily as it will be with make, CMake and so on, and if you modify your script it will auto rebuild itself and launch without developer intervention, this allow a more managable developing process.<br>

And of course, **this is still C**, you can execute compilation time code or preprocessing code, macro, code editing or code modifications/generation directly in your build system, customising whatever you want and do whatever you need just in C.<br><br>

From a user's perspective side, to begin building he just need to compile the build script ( which comes down to a single line of compilation as shown before ) and then launch the build script, *without installing or requiring special executable first* in order to proceed, all of that by just having a C compiler. 

