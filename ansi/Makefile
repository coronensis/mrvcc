
all: acc tst

acc: lexer.l parser.y
	yacc -v -d parser.y
	lex lexer.l
	gcc -I. lex.yy.c y.tab.c -o acc -lfl -lm

tst:
	cat ../tests/sample.c | ./acc

clean:
	rm -f *.o *.bin y.tab.c y.tab.h y.output lex.yy.c acc

