#include "AST.h"

static char *code_name(enum code op);

void printAST(AST *p)
{
    if(p == NULL){
	printf("()");
	return;
    }
    switch(p->op){
    case NUM:
	printf("%d",p->val);
	break;
    case SYM:
	printf("'%s'",p->sym->name);
	break;
    case LIST:
	printf("(LIST ");
	while(p != NULL){
	    printAST(p->left);
	    p = p->right;
	    if(p != NULL) printf(" ");
	}
	printf(")");
	break;
    default:
	printf("(%s ",code_name(p->op));
	printAST(p->left);
	printf(" ");
	printAST(p->right);
	printf(")");
    }
    fflush(stdout);
}

static char *code_name(enum code op)
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
    case GET_ARRAY_OP:	return "GET_ARRAY_OP";
    case SET_ARRAY_OP: 	return "SET_ARRAY_OP";
    case CALL_OP: 	return "CALL_OP";
    case PRINTLN_OP: 	return "PRINTLN_OP";
    case IF_STATEMENT:	return "IF_STATEMENT";
    case BLOCK_STATEMENT: return "BLOCK_STATEMENT";
    case RETURN_STATEMENT: return "RETURN_STATEMENT";
    case WHILE_STATEMENT: return "WHILE_STATEMENT";
    case FOR_STATEMENT: return "FOR_STATEMENT";
    default: return "???";
    }
}
