# dicc
A compiler for a subset of C written in C.
## Usage
To use the compiler, you first have to compile it. You can use the compile.sh script for that. This will create
the compiler executable 'dicc'. <br/>
To compile a program using dicc, run it and pass as command line argument its file name, like: ./dicc [name].c <br/>
Example: __./dicc test.c__ <br/>
This will create an x86_64 [name].s assembly file. To create an executable out of that, you just use the gcc assembler: <br/>
__gcc [name].s__
I also have included a test.sh script for ease of use for some test file named test.c
