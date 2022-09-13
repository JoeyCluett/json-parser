#!/bin/bash

# optimized
gcc -o main main.c json-parser.c json-parser-util.c -std=c11 -O2

# valgrind build
##gcc -o main main.c json-parser.c -fPIE -lm -I. -std=c11 -O0 -DTRACE_ON_EXIT -g

