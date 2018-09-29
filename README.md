# dicc
A compiler for a subset of C written in C. <br/>
This is a C to x86_64 assembly compiler. <br/>
It is structured as a standard 3-phase compiler. That means, it has a very simple lexer, and RD (recursive-descent) parser
and code generator for x86_64.
Thanks to https://github.com/georgeLs for his contribution to the project.

## Usage
To use the compiler, you first have to compile it. You can use the compile.sh script for that. This will create
the compiler executable 'dicc'. <br/>
To compile a program using dicc, run it and pass as command line argument its file name, like: ./dicc [name].c <br/>
Example: __./dicc test.c__ <br/>
This will create an x86_64 [name].s assembly file. To create an executable out of that, you just use some assembler, like gcc:
__gcc [name].s__ <br/> <br/>
I also have included a test.sh script for ease of use with some test file named test.c

## Compiler features
Currently, it supports:

* integers (no other types yet)
* logical NOT (`!a`)
* bitwise NOT (`~a`)
* logical AND (`a && b`)
* logical OR (`a || b`)
* addition, subtraction, multiplication, division (`+`, `-`, `*`, `/`)
* comparison operators (`<`,`<=`,`>`,`>=`,`==`)
* Any binary expression that combines any of the above expressions.
* C89 style comments (`/* ... */`)
* Only one function, main
* return statement
* custom print statement (not in any C standard)
* if/else statements (`if (exp) { stat1; stat2; ... } else { stat11; stat12; ... }`)
* local variables:
    It has function scope only and declare anywhere, but not block scope yet.
* while loops (`while (exp) { stat1; stat2; ... }`) whith `break` and `continue`
* Possibly other things that I forget...

Generally, the initial target feature set was one, so that the compiler could compile any assignment 1 from
the course Introduction to Programming: http://cgi.di.uoa.gr/~ip/ <br/>
There are still some things missing. The main one is preprocessor support and also some not-so-important features
(like support for all loop types or else if statements). However, it seems that you can compile any assigment 1 source code
with little modification.

## Educational features
The main goal of this project is to make a descent C compiler written in C, totally from scratch.
Currently, apart from the heavy commenting, the compiler tries to put a clear boundary between each phase of compilation.
At first, the lexer is run, and outputs its results. Its output is then passed to the parser whose contents are also shown.
Finally, code generator takes the output from the parser as input and spits out assembly.
Feel free to ask explanation for any part of the code.

## Reliability and possible improvements
Although the compiler has been tested somewhat, it's still far from adequately stable and reliable and it's definetely
not supposed to be used in production.
For any suggestions, please contact me.
