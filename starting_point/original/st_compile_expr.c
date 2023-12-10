#include "AST.h"
#include "st_code.h"
#include "st_compile.h"

static void printFunc(AST *args);

void  compileExpr(AST *p)
{
    if(p == NULL) return;

    switch(p->op){
    case NUM:
        genCodeI(PUSHI,p->val);
	return;
    case SYM:
	compileLoadVar(getSymbol(p));
	return;
    case EQ_OP:
	compileStoreVar(getSymbol(p->left),p->right);
	return;
    case PLUS_OP:
	compileExpr(p->left);
	compileExpr(p->right);
	genCode(ADD);
	return;
    case MINUS_OP:
	compileExpr(p->left);
	compileExpr(p->right);
	genCode(SUB);
	return;
    case MUL_OP:
	compileExpr(p->left);
	compileExpr(p->right);
	genCode(MUL);
	return;
    case LT_OP:
	compileExpr(p->left);
	compileExpr(p->right);
	genCode(LT);
	return;
    case GT_OP:
	compileExpr(p->left);
	compileExpr(p->right);
	genCode(GT);
	return;

    case CALL_OP:
	compileCallFunc(getSymbol(p->left),p->right);
	return;

    case PRINTLN_OP:
	printFunc(p->left);
	return;

    case GET_ARRAY_OP:
	/* not implemented */
    case SET_ARRAY_OP:
	/* not implemented */

    default:
	error("unknown operater/statement");
    }
}

static void printFunc(AST *args)
{
    compileExpr(getNth(args,1));
    genCodeS(PRINTLN,getNth(args,0)->str);
}

/*
 * global variable
 */
void declareVariable(Symbol *vsym,AST *init_value)
{
    /* not implemented */
}

/* 
 * Array
 */
void declareArray(Symbol *a, AST *size)
{
    /* not implemented */
}


