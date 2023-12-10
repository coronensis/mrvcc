#include "AST.h"
#include "reg_code.h"
#include "reg_compile.h"

int envp = 0;
Environment Env[MAX_ENV];

int label_counter = 0;
int local_var_pos;
int tmp_counter = 0;

void compileStoreVar(Symbol *var,int r)
{
    int i;
    for(i = envp-1; i >= 0; i--){
	if(Env[i].var == var){
	    switch(Env[i].var_kind){
	    case VAR_ARG:
		genCode2(STOREA,r,Env[i].pos);
		return;
	    case VAR_LOCAL:
		genCode2(STOREL,r,Env[i].pos);
		return;
	    }
	}
    }
    error("undefined variable\n");
}

void compileLoadVar(int target, Symbol *var)
{
    int i;
    for(i = envp-1; i >= 0; i--){
	if(Env[i].var == var){
	    switch(Env[i].var_kind){
	    case VAR_ARG:
		genCode2(LOADA,target,Env[i].pos);
		return;
	    case VAR_LOCAL:
		genCode2(LOADL,target,Env[i].pos);
		return;
	    }
	}
    }
    error("undefined variable\n");
}

void defineFunction(Symbol *fsym,AST *params,AST *body)
{
    int param_pos;

    initGenCode();
    envp = 0;
    param_pos = 0;
    local_var_pos = 0;
    for( ; params != NULL; params = getNext(params)){
	Env[envp].var = getSymbol(getFirst(params));
	Env[envp].var_kind = VAR_ARG;
	Env[envp].pos = param_pos++;
	envp++;
    }
    compileStatement(body);
    genFuncCode(fsym->name,local_var_pos);
    envp = 0;  /* reset */
}

void compileStatement(AST *p)
{
    if(p == NULL) return;
    switch(p->op){
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
    case FOR_STATEMENT:
	compileFor(getNth(p->left,0),getNth(p->left,1),getNth(p->left,2),
		   p->right);
	break;
    default:
	compileExpr(-1,p);
    }
}

void compileBlock(AST *local_vars,AST *statements)
{
    int envp_save;

    envp_save = envp;
    for(  ; local_vars != NULL; local_vars = getNext(local_vars)){
	Env[envp].var = getSymbol(getFirst(local_vars));
	Env[envp].var_kind = VAR_LOCAL;
	Env[envp].pos = local_var_pos++;
	envp++;
    }
    for( ; statements != NULL; statements = getNext(statements))
	compileStatement(getFirst(statements));
    envp = envp_save;
}

void compileReturn(AST *expr)
{
    int r;
    if(expr != NULL){
	r = tmp_counter++;
	compileExpr(r,expr);
    } else r = -1;
    genCode1(RET,r);
}

void compileCallFunc(int target, Symbol *f,AST *args)
{
    int narg;
    narg = compileArgs(args,0);
    genCodeS(CALL,target,narg,f->name);
}

int compileArgs(AST *args, int i)
{
    int r,n;

    if(args != NULL){
        n = compileArgs(getNext(args),i+1);
	r = tmp_counter++;
	compileExpr(r,getFirst(args));
	genCode2(ARG,r,i);
    } else return 0;
    return n+1;
}

void compileIf(AST *cond, AST *then_part, AST *else_part)
{
    int l1,l2;
    int r;

    r = tmp_counter++;
    compileExpr(r,cond);
    l1 = label_counter++;
    genCode2(BEQ0,r,l1);
    compileStatement(then_part);
    if(else_part != NULL){
	l2 = label_counter++;
	genCode1(JUMP,l2);
	genCode1(LABEL,l1);
	compileStatement(else_part);
	genCode1(LABEL,l2);
    } else {
	genCode1(LABEL,l1);
    }
}

void compileWhile(AST *cond,AST *body)
{
    int l1,l2;
    int r;

    l1 = label_counter++;
    l2 = label_counter++;
    r = tmp_counter++;

    genCode1(LABEL,l1);
    compileExpr(r,cond);
    genCode2(BEQ0,r,l2);
    compileStatement(body);
    genCode1(JUMP,l1);
    genCode1(LABEL,l2);
}

void compileFor(AST *init,AST *cond,AST *iter,AST *body)
{
    /* not implemented */
}


