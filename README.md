# builder.h 

builder.h is a simple header to create build script directly from C. It can handle asynch and synch building option by using simple command line struct to compose the desired command. <br>
It can also act as a simple process spawner directly implemented in C in a **header only library**. <br>


## How to build and use builder.h

builder.h is a header only library, it include both the declaration and implementation and you can access the implementation of the functions by declaring **BUILDER_IMP** before including builder.h<br>

```C

#define BUILDER_IMP
#include "builder.h"

int main(){
    // code here
}

```
<br>

It's suggested to use builder.h in a standalone implementations ( for example in a file named *builder.c* ), in that way you can bootstrap the build system into an executable that can be launched to 
begin with the compilation. <br><br>
If an update to the build script is needed you can modify *builder.c* and bootstrap it again, after that the build of your project can begin.


## Why using an integrated build system instead of makefile 

Having a build system for complex project is important and you can't avoid it if the complexity is growing fast, but that comes with the cost of having a separate program that can build your application, 
for example by using **make** with a dedicated makefile. <br>
You now have a dependency on **make** and require the build client to have this separate program installed before even begin with the compilation. <br><br>

builder.h require a bootstrapping process before launching the build since the build system is written in C, but for this reason the entire project depend only on the C compiler, reducing the risk of having an external program to start 
the compilation. **You just need a C compiler**. <br>
Beside that, you are working with a fully functional programming language that is acting as a build system, it means you can infact execute code or scripts while launching compilation command, for example you 
can include a "preprocessing" inside your *builder.c* that perform some work on the source code before launching the compilation, the possibilities are endless and can be all implemented in a build system written in 
the same language as your project is written with, or generally require just a C compiler to bootstrap the building process, and it's easy to have one instead of gamble on the existence of make inside the target system.
