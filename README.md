# builder.h 

builder.h is a single header library made to create build scripts directly in C. It can handle async and synch building option, all by using simple command line struct to compose different commands. <br>
It can act as a process spawner directly implemented in C. <br>


## How to build and use builder.h

builder.h is a header only library, it include both the declaration and implementation, and if you want to create a build script you must declare **BUILDER_IMP** before including *builder.h*: <br>

```C

#define BUILDER_IMP
#include "builder.h"

int main(){
	auto_rebuild("build.c", "build");
    // code here
}

```

By creating a separate file, the compilation process for a build script comes down to a single compiler line:  

```C

cc build.c -o build // if we assume the build file is called build.c
./build             // start build script 

```

<br>

It's suggested to use builder.h in a standalone implementation ( for example in a file named *builder.c* ), after that to begin with the compilation you must bootstrap it at leat once ( if you don't use "auto_rebuild()" in your script you must bootstrap the script each time you modify it ). <br>

## Why such system? 

With complex projects and rising complexity having a build system is almost mandatory, some developers use bash scripts, some use make file, with the only goal of **organise and control the build operation**. But having a build system has the disadvantage of introducing yet another dependency, which per se is not a bad thing, but it forces the client who want to build your project on relying on a **required** program in order to begin with the compilation.

Builder.h is a library written in C that offer a toolkit to easily build projects, to execute code during compilation, while using a fully working programming language. With that **in order to build your project you just need a C compiler, that's it**. <br> 
If you script your build system correctly ( by using auto_rebuild() on top of your build script ) you just need to bootstrap it once, then you can just build your system as easily as it will be with systems like make, CMake and so on, and if you modify your script it will auto rebuild and launch itself without the user intervention. <br>
And of course, **this is still C**, you can execute compilation time code or preprocessing code, macro, code editing or modifications/generation directly in your build system, customising whatever you want and do whatever you need just in C.<br><br>

From a user's perspective side, to build your project it just need to compile your build script ( which comes down to a single line of compilation as shown before ) and then launch the build script, *without installing or requiring special executable first* in order to proceed, all of that by just having a C compiler. 

