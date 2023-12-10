all: clean single_file_compiler tst

single_file_compiler: single_file_compiler.y
	yacc -v -d single_file_compiler.y
	gcc -std=gnu99 -Og -ggdb3 -Wall -Wextra -pedantic -Werror -Wundef -Wshadow -Wunused-parameter -Warray-bounds -Wredundant-decls -Wunreachable-code y.tab.c -o single_file_compiler

tst:
	./single_file_compiler sample.c >sample.s
	gcc -m32 sample.s println.c -o sample
	./sample

clean:
	rm -f single_file_compiler *.o y.tab.c y.tab.h y.output sample.s sample
