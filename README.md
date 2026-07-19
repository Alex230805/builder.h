# builder.h 

builder.h is a single header library made to create build scripts directly in C. It can handle async and sync building mode, all by using simple command line struct to compose different commands. <br>
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

cc builder.c -o builder // if we assume the builder file is called builder.c
./builder             // start builder script 

```

<br>

It's suggested to use builder.h in a standalone file implementation ( for example in a file named *builder.c* ), after that to begin with the compilation the system must be bootstrapped at leat once ( if you don't use "auto_rebuild()" in your script you must bootstrap it each time you made modifications ). <br>

## Why such system? 

With complex projects and rising complexity having a build system is almost mandatory, some developers use bash scripts, some use make files, with the only goal of **organise and control the build operation/s**. But having a build system has the disadvantage of introducing yet another dependency, which is not a bad thing per se, but it forces the client who build your project to have a **required** program in order to even begin with the compilation.

Builder.h is a library written in C that offer a toolkit to easily build projects, to execute code during compilation, while using a fully working programming language. With that **in order to build your project you just need a C compiler, that's it**. <br> 
If you script your build system correctly ( by using auto_rebuild() on top of your build script ) you just need to bootstrap it once, then you can just build your system as easily as it will be with make, CMake and so on, and if you modify your script it will auto rebuild itself and launch without developer intervention, this allow a more managable developing process.<br>

And of course, **this is still C**, you can execute compilation time code or preprocessing code, macro, code editing or code modifications/generation directly in your build system, customising whatever you want and do whatever you need just in C.<br><br>

From a user's perspective side, to begin building he just need to compile the build script ( which comes down to a single line of compilation as shown before ) and then launch the build script, *without installing or requiring special executable first* in order to proceed, all of that by just having a C compiler. 

