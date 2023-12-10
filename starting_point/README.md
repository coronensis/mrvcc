# tiny-c

This is the starting point from which the compiler will be build.
The original source I found the following location:

https://www.hpcs.cs.tsukuba.ac.jp/~msato/lecture-note/comp-lecture/

In the file tiny-c.tgz

From that I put together a single-file, fully working, compiler for
a subset of the C language, which targets the x86 instruction set
and is less than 1000 lines of code.

This will serve as the basis for all further development.

To build the compiler you will need flex and yacc installed as well
as gcc-multilib as the output of the compiler is 32 bit assembly language
that must be compiled with the -m32 flag of gcc.

Copyright: There is no license file or copyright notice in the archive
of the original source code, so I assume it is OK to use it for an open
source project. If I'm wrong please notfiy me and I will remove it.
As for my changes: You can do with them whatever you like. No restrictions.

