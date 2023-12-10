#include "AST.h"
#include "reg_code.h"
#include "reg_compile.h"

static void printFunc(AST *args);

void compileExpr(int target, AST *p)
{
    int r1,r2;
    
    if(p == NULL) return;

    switch(p->op){
    case NUM:
        genCode2(LOADI,target,p->val);
	return;
    case SYM:
	compileLoadVar(target,getSymbol(p));
	return;
    case EQ_OP:
	if(target != -1) error("assign has no value");
	r1 = tmp_counter++; 
	compileExpr(r1,p->right);
	compileStoreVar(getSymbol(p->left),r1);
	return;

    case PLUS_OP:
	r1 = tmp_counter++; r2 = tmp_counter++;
	compileExpr(r1,p->left);
	compileExpr(r2,p->right);
	genCode3(ADD,target,r1,r2);
	return;
    case MINUS_OP:
	r1 = tmp_counter++; r2 = tmp_counter++;
	compileExpr(r1,p->left);
	compileExpr(r2,p->right);
	genCode3(SUB,target,r1,r2);
	return;
    case MUL_OP:
	r1 = tmp_counter++; r2 = tmp_counter++;
	compileExpr(r1,p->left);
	compileExpr(r2,p->right);
	genCode3(MUL,target,r1,r2);
	return;
    case LT_OP:
	r1 = tmp_counter++; r2 = tmp_counter++;
	compileExpr(r1,p->left);
	compileExpr(r2,p->right);
	genCode3(LT,target,r1,r2);
	return;
    case GT_OP:
	r1 = tmp_counter++; r2 = tmp_counter++;
	compileExpr(r1,p->left);
	compileExpr(r2,p->right);
	genCode3(GT,target,r1,r2);
	return;
    case CALL_OP:
	compileCallFunc(target,getSymbol(p->left),p->right);
	return;

    case PRINTLN_OP:
	if(target != -1) error("println has no value");
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
    int l,r;

    l = genString(getNth(args,0)->str);
    r = tmp_counter++;
    compileExpr(r,getNth(args,1));
    genCode2(PRINTLN,r,l);
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






