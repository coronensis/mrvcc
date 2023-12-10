%{
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define FALSE 0
#define TRUE 1

#define MAX_SYMBOLS 100

#define LOADI 	 1	/* LOADI r,i */ /* load int */
#define LOADA 	 2	/* LOADA r,n *//* load arg */
#define LOADL	 3	/* LOADL r,n *//* load local var */
#define STOREA	 4	/* STOREA r,n *//* store arg */
#define STOREL	 5	/* STOREL r,n *//* store local var */
#define ADD 	 6	/* ADD t,r1,r2 */
#define SUB 	 7	/* SUB t,r1,r2 */
#define MUL 	 8	/* MUL t,r1,r2 */
#define GT		 9 	/* GT  t,r1,r2 */
#define LT		 10	/* LT  r,r1,r2 */
#define BEQ0	 11	/* BQ  r,L *//* branch if eq 0 */
#define JUMP	 12	/* JUMP L */
#define ARG      13	/* ARG r,n *//* set Argument */
#define CALL	 14	/* CALL r,func */
#define RET		 15	/* RET r *//* return */
#define PRINT	 16	/* PRINTLN r,L *//* println function */
#define LABEL 	 17	/* LABEL L *//* label */

#define LOADS    18	/* load string label */

#define LOADADDR 19
#define LOAD     20
#define STORE	 21

#define VAR_ARG   0
#define VAR_LOCAL 1

#define MAX_ENV   100

#define MAX_CODE  100

#define N_REG     4
#define N_SAVE    4

#define TMP_OFF(i)		 	-((i+1)+1)*4
#define LOCAL_VAR_OFF(i) 	-(N_SAVE+1+(i+1))*4
#define ARG_OFF(i)			((i)+2)*4

#define REG_AX    0
#define REG_BX    1
#define REG_CX    2
#define REG_DX    3

enum code {
	LIST,
	NUM,
	STR,
	SYM,
	EQ_OP,
	PLUS_OP,
	MINUS_OP,
	MUL_OP,
	LT_OP,
	GT_OP,
	CALL_OP,
	PRINTLN_OP,
	IF_STATEMENT,
	BLOCK_STATEMENT,
	RETURN_STATEMENT,
	WHILE_STATEMENT
};

typedef struct abstract_syntax_tree {
	enum code op;
	struct abstract_syntax_tree *left;
	struct abstract_syntax_tree *right;

	struct symbol *sym;
	char *str;
	int val;
} AST;

typedef struct symbol {
	char *name;
} Symbol;

struct _code {
	int opcode;
	int operand1;
	int operand2;
	int operand3;
	char *s_operand;
} Codes[MAX_CODE];

typedef struct env {
    Symbol *var;
    int var_kind;
    int pos;
} Environment;

char *tmpRegName[N_REG] = { "%eax", "%ebx", "%ecx", "%edx" };
int tmpRegState[N_REG];
int tmpRegSave[N_SAVE];

Environment Env[MAX_ENV];
Symbol SymbolTable[MAX_SYMBOLS];

int envp = 0;
int label_counter = 0;
int local_var_pos;
int tmp_counter = 0;
int n_code = 0;
int n_symbols = 0;

void compileStatement(AST *p);
void compileExpr(int target, AST *p);
int yylex();

void yyerror() {
    printf("syntax error!\n");
    exit(1);
}

void error(char *msg) {
    fprintf(stderr,"compiler error: %s",msg);
    exit(1);
}

// --------- syntax tree ---------

AST *makeNum(int val) {
	AST *p = (AST *)malloc(sizeof(AST));
	p->op = NUM;
	p->val = val;
	return p;
}

AST *makeStr(char *s) {
	AST *p = (AST *)malloc(sizeof(AST));
	p->op = STR;
	p->str = s;
	return p;
}

AST *makeAST(enum code op,AST *left,AST *right) {
	AST *p = (AST *)malloc(sizeof(AST));
	p->op = op;
	p->right = right;
	p->left = left;
	return p;
}

AST *getNth(AST *p,int nth) {
	if (p->op != LIST) {
		fprintf(stderr,"bad access to list\n");
		exit(1);
	}
	if(nth > 0)
		return getNth(p->right, nth - 1);
	else
		return p->left;
}

AST *addLast(AST *l,AST *p) {
	AST *q;
	if(l == NULL)
		return makeAST(LIST,p,NULL);
	q = l;
	while (q->right != NULL)
		q = q->right;
	q->right = makeAST(LIST,p,NULL);
	return l;
}

AST *getNext(AST *p) {
	if (p->op != LIST) {
		fprintf(stderr,"bad access to list\n");
		exit(1);
	}
	else
		return p->right;
}

char *code_name(enum code op)
{
    switch(op){
    case LIST: 	return "LIST";
    case NUM: 	return "NUM";
    case SYM: 	return "SYM";
    case EQ_OP: 	return "EQ_OP";
    case PLUS_OP:	return "PLUS_OP";
    case MINUS_OP: 	return "MINUS_OP";
    case MUL_OP:	return "MUL_OP";
    case LT_OP:	return "LT_OP";
    case GT_OP:	return "GT_OP";
    case CALL_OP: 	return "CALL_OP";
    case PRINTLN_OP: 	return "PRINTLN_OP";
    case IF_STATEMENT:	return "IF_STATEMENT";
    case BLOCK_STATEMENT: return "BLOCK_STATEMENT";
    case RETURN_STATEMENT: return "RETURN_STATEMENT";
    case WHILE_STATEMENT: return "WHILE_STATEMENT";
    default: return "???";
    }
}

void printAST(AST *p) {
	if(p == NULL) {
		fprintf(stderr, "()");
		return;
	}
	switch(p->op){
		case NUM:
			fprintf(stderr,"%d",p->val);
			break;
		case SYM:
			fprintf(stderr, "'%s'",p->sym->name);
			break;
		case LIST:
			fprintf(stderr, "(LIST ");
			while(p != NULL){
				printAST(p->left);
				p = p->right;
				if(p != NULL)
					fprintf(stderr, " ");
			}
			fprintf(stderr, ")");
			break;
		default:
			fprintf(stderr, "(%s ",code_name(p->op));
			printAST(p->left);
			fprintf(stderr, " ");
			printAST(p->right);
			fprintf(stderr, ")");
	}
    fflush(stdout);
}


// --------- symbol table ---------

Symbol *lookupSymbol(char *name) {
	int i;
	Symbol *sp = NULL;

	for (i = 0; i < n_symbols; i++)
		if (strcmp(SymbolTable[i].name,name) == 0) {
			sp = &SymbolTable[i];
			break;
		}
	if (sp == NULL) {
		sp = &SymbolTable[n_symbols++];
		sp->name = strdup(name);
	}
	return sp;
}

AST *makeSymbol(char *name) {
	AST *p = (AST *)malloc(sizeof(AST));
	p->op = SYM;
	p->sym = lookupSymbol(name);
	return p;
}

Symbol *getSymbol(AST *p) {
	if (p->op != SYM) {
		fprintf(stderr,"bad access to symbol\n");
		exit(1);
	}
	else
		return p->sym;
}

// --------- register ---------

void initTmpReg() {
	int i;
	for(i = 0; i < N_REG; i++)
		tmpRegState[i] = -1;
	for(i = 0; i < N_SAVE; i++)
		tmpRegSave[i] = -1;
}

int getFreeReg(int r) {
	int i;
	for(i = 0; i < N_REG; i++)
		if(tmpRegState[i] < 0){
			tmpRegState[i] = r;
			return i;
		}
	error("no temp reg");
	return -1;
}

int useReg(int r) {
	int i,rr;
	for(i = 0; i < N_REG; i++)
		if (tmpRegState[i] == r)
			return i;
	/* not found in register, then restore from save area. */
	for(i = 0; i < N_SAVE; i++)
		if (tmpRegSave[i] == r) {
			rr = getFreeReg(r);
			tmpRegSave[i] = -1;
			/* load into regsiter */
			printf("\tmovl\t%d(%%ebp),%s\n",TMP_OFF(i),tmpRegName[rr]);
			return rr;
		}
	error("reg is not found");
	return -1;
}

void saveReg(int reg) {
	int i;

	if (tmpRegState[reg] < 0)
		return;
	for (i = 0; i < N_SAVE; i++)
		if (tmpRegSave[i] < 0) {
			printf("\tmovl\t%s,%d(%%ebp)\n",tmpRegName[reg],TMP_OFF(reg));
			tmpRegSave[i] = tmpRegState[reg];
			tmpRegState[reg] = -1;
			return;
		}
	error("no temp save");
}

void saveAllRegs() {
	int i;
	for (i = 0; i < N_REG; i++)
		saveReg(i);
}
void assignReg(int r, int reg) {
	if (tmpRegState[reg] == r)
		return;
	saveReg(reg);
	tmpRegState[reg] = r;
}

void freeReg(int reg) {
	tmpRegState[reg] = -1;
}

// --------- generate ---------

void genCode1(int opcode,int operand1) {
	Codes[n_code].operand1 = operand1;
	Codes[n_code++].opcode = opcode;
}

void genCode2(int opcode,int operand1, int operand2) {
	Codes[n_code].operand1 = operand1;
	Codes[n_code].operand2 = operand2;
	Codes[n_code++].opcode = opcode;
}

void genCode3(int opcode,int operand1, int operand2, int operand3) {
	Codes[n_code].operand1 = operand1;
	Codes[n_code].operand2 = operand2;
	Codes[n_code].operand3 = operand3;
	Codes[n_code++].opcode = opcode;
}

void genCodeS(int opcode,int operand1, int operand2, char *s) {
	Codes[n_code].operand1 = operand1;
	Codes[n_code].operand2 = operand2;
	Codes[n_code].s_operand = s;
	Codes[n_code++].opcode = opcode;
}

int genString(char *s) {
	int l = label_counter++;
	printf("\t.section\t.rodata\n");
	printf(".LC%d:\n",l);
	printf("\t.string \"%s\"\n",s);
	return l;
}

void genFuncCode(char *entry_name, int n_local) {
	int i;
	int opd1,opd2,opd3;
	int r,r1,r2;
	char *opds;
	int ret_lab,l1,l2;
	int frame_size;

	/* function header */
	puts("\t.text");       	           				/* .text         			 */
	puts("\t.align\t4");        	   				/* .align 4      			 */
	printf("\t.globl\t%s\n", entry_name);    		/* .globl <name>			 */
	printf("\t.type\t%s,@function\n", entry_name);	/* .type <name>,@function	 */
	printf("%s:\n", entry_name);             		/* <name>:					 */
	printf("\tpushl\t%%ebp\n");
	printf("\tmovl\t%%esp,%%ebp\n");
	frame_size = -LOCAL_VAR_OFF(n_local);
	ret_lab = label_counter++;
	printf("\tsubl\t$%d,%%esp\n",frame_size);
	printf("\tmovl\t%%ebx,-4(%%ebp)\n");

	initTmpReg();

	for (i = 0; i < n_code; i++) {
		opd1 = Codes[i].operand1;
		opd2 = Codes[i].operand2;
		opd3 = Codes[i].operand3;
		opds = Codes[i].s_operand;

		switch(Codes[i].opcode){
		case LOADI:
			if(opd1 < 0)
				break;
			r = getFreeReg(opd1);
			printf("\tmovl\t$%d,%s\n",opd2,tmpRegName[r]);
			break;
		case LOADA:	/* load arg */
			if(opd1 < 0)
				break;
			r = getFreeReg(opd1);
			printf("\tmovl\t%d(%%ebp),%s\n",ARG_OFF(opd2),tmpRegName[r]);
			break;
		case LOADL:	/* load local */
			if(opd1 < 0)
				break;
			r = getFreeReg(opd1);
			printf("\tmovl\t%d(%%ebp),%s\n",LOCAL_VAR_OFF(opd2),tmpRegName[r]);
			break;
		case STOREA:	/* store arg */
			r = useReg(opd1);
			freeReg(r);
			printf("\tmovl\t%s,%d(%%ebp)\n",tmpRegName[r],ARG_OFF(opd2));
			break;
		case STOREL:	/* store local */
			r = useReg(opd1);
			freeReg(r);
			printf("\tmovl\t%s,%d(%%ebp)\n",tmpRegName[r],LOCAL_VAR_OFF(opd2));
			break;
		case BEQ0:	/* conditional branch */
			r = useReg(opd1);
			freeReg(r);
			printf("\tcmpl\t$0,%s\n",tmpRegName[r]);
			printf("\tje\t.L%d\n",opd2);
			break;
		case LABEL:
			printf(".L%d:\n",Codes[i].operand1);
			break;
		case JUMP:
			printf("\tjmp\t.L%d\n",Codes[i].operand1);
			break;
		case CALL:
			saveAllRegs();
			printf("\tcall\t%s\n",opds);
			if(opd1 < 0)
				break;
			assignReg(opd1,REG_AX);
			printf("\tadd $%d,%%esp\n",opd2*4);
			break;
		case ARG:
			r = useReg(opd1);
			freeReg(r);
			printf("\tpushl\t%s\n",tmpRegName[r]);
			break;
		case RET:
			r = useReg(opd1);
			freeReg(r);
			if(r != REG_AX)
				printf("\tmovl\t%s,%%eax\n",tmpRegName[r]);
			printf("\tjmp .L%d\n",ret_lab);
			break;
		case ADD:
			r1 = useReg(opd2); r2 = useReg(opd3);
			freeReg(r1);
			freeReg(r2);
			if(opd1 < 0)
				break;
			assignReg(opd1,r1);
			printf("\taddl\t%s,%s\n",tmpRegName[r2],tmpRegName[r1]);
			break;
		case SUB:
			r1 = useReg(opd2); r2 = useReg(opd3);
			freeReg(r1);
			freeReg(r2);
			if(opd1 < 0)
				break;
			assignReg(opd1,r1);
			printf("\tsubl\t%s,%s\n",tmpRegName[r2],tmpRegName[r1]);
			break;
		case MUL:
			r1 = useReg(opd2); r2 = useReg(opd3);
			freeReg(r1);
			freeReg(r2);
			if(opd1 < 0)
				break;
			assignReg(opd1,REG_AX);
			saveReg(REG_DX);
			if(r1 != REG_AX)
				printf("\tmovl %s,%s\n",tmpRegName[r1],tmpRegName[REG_AX]);
			printf("\timull\t%s,%s\n",tmpRegName[r2],tmpRegName[REG_AX]);
			break;
		case LT:
			r1 = useReg(opd2); r2 = useReg(opd3);
			freeReg(r1);
			freeReg(r2);
			if(opd1 < 0)
				break;
			r = getFreeReg(opd1);
			l1 = label_counter++;
			l2 = label_counter++;
			printf("\tcmpl\t%s,%s\n",tmpRegName[r2],tmpRegName[r1]);
			printf("\tjl .L%d\n",l1);
			printf("\tmovl\t$0,%s\n",tmpRegName[r]);
			printf("\tjmp .L%d\n",l2);
			printf(".L%d:\tmovl\t$1,%s\n",l1,tmpRegName[r]);
			printf(".L%d:",l2);
			break;
		case GT:
			r1 = useReg(opd2); r2 = useReg(opd3);
			freeReg(r1);
			freeReg(r2);
			if(opd1 < 0)
				break;
			r = getFreeReg(opd1);
			l1 = label_counter++;
			l2 = label_counter++;
			printf("\tcmpl\t%s,%s\n",tmpRegName[r2],tmpRegName[r1]);
			printf("\tjg .L%d\n",l1);
			printf("\tmovl\t$0,%s\n",tmpRegName[r]);
			printf("\tjmp .L%d\n",l2);
			printf(".L%d:\tmovl\t$1,%s\n",l1,tmpRegName[r]);
			printf(".L%d:",l2);
			break;

		case PRINT:
			r = useReg(opd1);
			freeReg(r);
			printf("\tpushl\t%s\n",tmpRegName[r]);
			printf("\tlea\t.LC%d,%s\n",opd2, tmpRegName[r]);
			printf("\tpushl\t%s\n",tmpRegName[r]);
			saveAllRegs();
			printf("\tcall\tprintln\n");
			printf("\taddl\t$8,%%esp\n");
			break;
		}
	}
	/* return sequence */
	printf(".L%d:\tmovl\t-4(%%ebp), %%ebx\n",ret_lab);
	printf("\tleave\n");
	printf("\tret\n");
}

// --------- compile ---------

void compileBlock(AST *local_vars,AST *statements) {
	int envp_save;
	envp_save = envp;
	for (;local_vars != NULL; local_vars = getNext(local_vars)) {
		Env[envp].var = getSymbol(getNth(local_vars, 0));
		Env[envp].var_kind = VAR_LOCAL;
		Env[envp].pos = local_var_pos++;
		envp++;
	}
	for (;statements != NULL; statements = getNext(statements))
		compileStatement(getNth(statements,0));
	envp = envp_save;
}

int compileArgs(AST *args, int i) {
	int n;
	if (args != NULL) {
		n = compileArgs(getNext(args), i + 1);
		int r = tmp_counter++;
		compileExpr(r,getNth(args, 0));
		genCode2(ARG,r,i);
	}
	else
		return 0;
	return n + 1;
}

void compileCallFunc(int target, Symbol *f,AST *args) {
	int narg;
	narg = compileArgs(args,0);
	genCodeS(CALL, target, narg, f->name);
}

void compileLoadStoreVar(Symbol *var,int r, int loadstore) {
	int i;
	for (i = envp-1; i >= 0; i--)
		if (Env[i].var == var)
			switch (Env[i].var_kind) {
			case VAR_ARG:
				if (loadstore == LOAD)
					genCode2(LOADA, r, Env[i].pos);
				else
					genCode2(STOREA,r,Env[i].pos);
				return;
			case VAR_LOCAL:
				if (loadstore == LOAD)
					genCode2(LOADL, r, Env[i].pos);
				else
					genCode2(STOREL, r, Env[i].pos);
				return;
			}
	error("undefined variable\n");
}

void printFunc(AST *args) {
	int l = genString(getNth(args,0)->str);
	int r = tmp_counter++;
	compileExpr(r,getNth(args,1));
	genCode2(PRINT,r,l);
}

void compileExpr(int target, AST *p) {
	int r1,r2;
	if (p == NULL)
		return;
	switch(p->op) {
	case NUM:
		genCode2(LOADI,target,p->val);
		return;
	case SYM:
		compileLoadStoreVar(getSymbol(p), target, LOAD);
		return;
	case EQ_OP:
		if(target != -1)
			error("assign has no value");
		r1 = tmp_counter++;
		compileExpr(r1,p->right);
		compileLoadStoreVar(getSymbol(p->left),r1, STORE);
		return;
	case PLUS_OP:
		r1 = tmp_counter++;
		r2 = tmp_counter++;
		compileExpr(r1,p->left);
		compileExpr(r2,p->right);
		genCode3(ADD,target,r1,r2);
		return;
	case MINUS_OP:
		r1 = tmp_counter++;
		r2 = tmp_counter++;
		compileExpr(r1,p->left);
		compileExpr(r2,p->right);
		genCode3(SUB,target,r1,r2);
		return;
	case MUL_OP:
		r1 = tmp_counter++;
		r2 = tmp_counter++;
		compileExpr(r1,p->left);
		compileExpr(r2,p->right);
		genCode3(MUL,target,r1,r2);
		return;
	case LT_OP:
		r1 = tmp_counter++;
		r2 = tmp_counter++;
		compileExpr(r1,p->left);
		compileExpr(r2,p->right);
		genCode3(LT,target,r1,r2);
		return;
	case GT_OP:
		r1 = tmp_counter++;
		r2 = tmp_counter++;
		compileExpr(r1,p->left);
		compileExpr(r2,p->right);
		genCode3(GT,target,r1,r2);
		return;
	case CALL_OP:
		compileCallFunc(target, getSymbol(p->left), p->right);
		return;
	case PRINTLN_OP:
		if(target != -1)
			error("println has no value");
		printFunc(p->left);
		return;
	default:
		error("unknown operater/statement");
	}
}

void compileReturn(AST *expr) {
	int r;
	if(expr != NULL) {
		r = tmp_counter++;
		compileExpr(r,expr);
	}
	else
		r = -1;
	genCode1(RET,r);
}

void compileIf(AST *cond, AST *then_part, AST *else_part) {
	int l1,l2;
	int r = tmp_counter++;
	compileExpr(r,cond);
	l1 = label_counter++;
	genCode2(BEQ0,r,l1);
	compileStatement(then_part);
	if (else_part != NULL) {
		l2 = label_counter++;
		genCode1(JUMP,l2);
		genCode1(LABEL,l1);
		compileStatement(else_part);
		genCode1(LABEL,l2);
	}
	else
		genCode1(LABEL,l1);
}

void compileWhile(AST *cond,AST *body) {
	int l1 = label_counter++;
	int l2 = label_counter++;
	int r = tmp_counter++;

	genCode1(LABEL,l1);
	compileExpr(r,cond);
	genCode2(BEQ0,r,l2);
	compileStatement(body);
	genCode1(JUMP,l1);
	genCode1(LABEL,l2);
}

void compileStatement(AST *p) {
	if (p == NULL)
		return;
	switch (p->op) {
	case BLOCK_STATEMENT:
		compileBlock(p->left,p->right);
		break;
	case RETURN_STATEMENT:
		compileReturn(p->left);
		break;
	case IF_STATEMENT:
		compileIf(p->left,getNth(p->right,0),getNth(p->right,1));
		break;
	case WHILE_STATEMENT:
		compileWhile(p->left,p->right);
		break;
	default:
		compileExpr(-1,p);
		break;
	}
}

void defineFunction(Symbol *fsym, AST *params, AST *body) {
	int param_pos;
	n_code = 0; // initGenCode();
	envp = 0;
	param_pos = 0;
	local_var_pos = 0;
	for (;params != NULL; params = getNext(params)) {
		Env[envp].var = getSymbol(getNth(params,0));
		Env[envp].var_kind = VAR_ARG;
		Env[envp].pos = param_pos++;
		envp++;
	}
	printAST(body);
	compileStatement(body);
	genFuncCode(fsym->name,local_var_pos);
	envp = 0;  /* reset */
}

// --------- parser grammar description ---------
%}

%union {
    AST *val;
}

%token NUMBER
%token SYMBOL
%token STRING
%token INT
%token IF
%token ELSE
%token RETURN
%token WHILE
%token PRINTLN

%right '='
%left '<' '>'
%left '+' '-'
%left '*'

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%type <val> parameter_list block local_vars symbol_list
%type <val> statements statement expr primary_expr arg_list
%type <val> SYMBOL NUMBER STRING

%start program
%%
program
	: /* empty */
	| external_definitions
	;
external_definitions
	: external_definition
	| external_definitions external_definition
	;
external_definition
	: SYMBOL parameter_list block  /* function definition */ { defineFunction(getSymbol($1), $2, $3); }
	;
parameter_list
	: '(' ')'												 { $$ = NULL; }
	| '(' symbol_list ')'									 { $$ = $2; }
	;
block
    : '{' local_vars statements '}'						 	 { $$ = makeAST(BLOCK_STATEMENT, $2, $3); }
	;
local_vars
	: /* NULL */											 { $$ = NULL; }
	| INT symbol_list ';'									 { $$ = $2; }
	;
symbol_list
	: SYMBOL												 { $$ = makeAST(LIST, $1, NULL); }
	| symbol_list ',' SYMBOL								 { $$ = addLast($1, $3); }
	;
statements
	: statement												 { $$ = makeAST(LIST, $1, NULL); }
	| statements statement									 { $$ = addLast($1, $2); }
	;
statement
	: expr ';'
	| block
	| IF '(' expr ')' statement %prec LOWER_THAN_ELSE		 { $$ = makeAST(IF_STATEMENT,$3, makeAST(LIST, $5, makeAST(LIST, NULL, NULL))); }
    | IF '(' expr ')' statement ELSE statement				 { $$ = makeAST(IF_STATEMENT,$3, makeAST(LIST, $5, makeAST(LIST, $7, NULL))); }
	| RETURN expr ';'										 { $$ = makeAST(RETURN_STATEMENT, $2, NULL); }
	| RETURN ';' 											 { $$ = makeAST(RETURN_STATEMENT, NULL, NULL); }
	| WHILE '(' expr ')' statement 							 { $$ = makeAST(WHILE_STATEMENT, $3, $5); }
	;
expr
	: primary_expr
	| SYMBOL '=' expr										 { $$ = makeAST(EQ_OP, $1, $3); }
	| expr '+' expr 										 { $$ = makeAST(PLUS_OP, $1, $3); }
	| expr '-' expr 										 { $$ = makeAST(MINUS_OP, $1, $3); }
	| expr '*' expr 										 { $$ = makeAST(MUL_OP, $1, $3); }
	| expr '<' expr 										 { $$ = makeAST(LT_OP, $1, $3); }
	| expr '>' expr											 { $$ = makeAST(GT_OP, $1, $3); }
	;
primary_expr
	: SYMBOL
	| NUMBER
	| STRING
	| SYMBOL '(' arg_list ')' 								{ $$ = makeAST(CALL_OP, $1, $3); }
	| SYMBOL '(' ')' 										{ $$ = makeAST(CALL_OP, $1, NULL); }
    | '(' expr ')' 											{ $$ = $2; }
	| PRINTLN  '(' arg_list ')' 							{ $$ = makeAST(PRINTLN_OP, $3, NULL); }
	;
arg_list
	: expr 													{ $$ = makeAST(LIST, $1, NULL); }
	| arg_list ',' expr 									{ $$ = addLast($1, $3); }
	;
%%
// ------------ Lexer ------------
char yytext[100];

int yylex() {
	int c,n;
	char *p;
	while (isspace(c = getc(stdin)))
		;
	switch(c) {
		case '+':
		case '-':
		case '*':
		case '>':
		case '<':
		case '(':
		case ')':
		case '{':
		case '}':
		case ';':
		case ',':
		case '=':
		case EOF:
			return c;
		case '"':
			p = yytext;
			while((c = getc(stdin)) != '"')
				*p++ = c;
			*p = '\0';
			yylval.val = makeStr(strdup(yytext));
			return STRING;
	}
	if(isdigit(c)) {
		n = 0;
		do {
			n = n*10 + c - '0';
			c = getc(stdin);
		} while(isdigit(c));

		ungetc(c,stdin);
		yylval.val = makeNum(n);
		return NUMBER;
	}
	if (isalpha(c)) {
		p = yytext;
		do {
			*p++ = c;
			c = getc(stdin);
		} while(isalpha(c));
		*p = '\0';
		ungetc(c,stdin);
		if (!strcmp(yytext,"int"))
			return INT;
		else if (!strcmp(yytext,"if"))
			return IF;
		else if (!strcmp(yytext,"else"))
			return ELSE;
		else if (!strcmp(yytext,"return"))
			return RETURN;
		else if (!strcmp(yytext,"while"))
			return WHILE;
		else if (!strcmp(yytext,"println"))
			return PRINTLN;
		else {
			yylval.val = makeSymbol(yytext);
			return SYMBOL;
		}
	}
	fprintf(stderr,"bad char '%c'\n",c);
	exit(1);
}

int main(int argc, char **argv) {
	if (argc > 1)
		stdin = fopen(argv[1], "r");
    yyparse();
    return 0;
}

