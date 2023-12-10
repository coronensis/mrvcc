#
# tiny C interpreter/compiler Makefile
#

CFLAGS = -g

PARSER_SRC = AST.h AST.c cparser.y clex.c

INTERP_SRC = interp_main.c interp.c interp_expr.c

ST_SRC = compiler_main.c st_compile.c st_compile_expr.c \
	st_code_gen.c st_code.c

REG_SRC = compiler_main.c reg_compile.c reg_compile_expr.c\
	x86_code_gen.c

INTERP_OBJ = interp_main.o AST.o cparser.o interp.o interp_expr.o

ST_OBJ = compiler_main.o AST.o cparser.o st_compile.o st_compile_expr.o\
	st_code_gen.o st_code.o

REG_OBJ = compiler_main.o AST.o cparser.o reg_compile.o reg_compile_expr.o\
	x86_code_gen.o

PROGRAM = tiny-c-run tiny-cc-st tiny-cc-x86 st_machine

all: $(PROGRAM)

tiny-c-run: $(INTERP_OBJ)
	$(CC) $(CFLAGS) -o tiny-c-run $(INTERP_OBJ)

tiny-cc-st: $(ST_OBJ)
	$(CC) $(CFLAGS) -o tiny-cc-st $(ST_OBJ)

tiny-cc-x86: $(REG_OBJ)
	$(CC) $(CFLAGS) -o tiny-cc-x86 $(REG_OBJ)

cparser.o: cparser.y clex.c
	yacc cparser.y
	mv y.tab.c cparser.c
	$(CC) $(CFLAGS) -c cparser.c

st_machine: st_machine.o st_code.o
	$(CC) $(CFLAGS) -o st_machine st_machine.o st_code.o

tiny-c.tgz: 
	tar zcvf tiny-c.tgz Makefile $(PARSER_SRC) $(ST_SRC) $(REG_SRC) st_machine.c

clean:
	/bin/rm -f *.o $(PROGRAM) cparser.c
