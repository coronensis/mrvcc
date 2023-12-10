all: mrvcc

mrvcc: parser.y lexer.l compiler.c
	lex lexer.l
	yacc -v -d parser.y
	gcc -std=gnu99 -Og -ggdb3 -Wall -Wextra -pedantic -Werror -Wundef -Wshadow -Wunused-parameter -Warray-bounds -Wunreachable-code y.tab.c -o mrvcc

tst:
	./mrvcc tests/sample.c >sample.s
	gcc -m32 sample.s println.c -o sample
	./sample

rv:
	./mrvcc demo.c >demo.s

clean:
	rm -f lex.yy.c y.tab.c y.tab.h y.output *.o mrvcc sample.s sample

