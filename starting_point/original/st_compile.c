#include "AST.h"
#include "st_code.h"
#include "st_compile.h"

int envp = 0;
Environment Env[MAX_ENV];

int label_counter = 0;
int local_var_pos;

void compileStoreVar(Symbol *var,AST *v)
{
    int i;
    compileExpr(v);
    for(i = envp-1; i >= 0; i--){
	if(Env[i].var == var){
	    switch(Env[i].var_kind){
	    case VAR_ARG:
		genCodeI(STOREA,Env[i].pos);
		return;
	    case VAR_LOCAL:
		genCodeI(STOREL,Env[i].pos);
		return;
	    }
	}
    }
    error("undefined variable\n");
}

void compileLoadVar(Symbol *var)
{
    int i;
    for(i = envp-1; i >= 0; i--){
	if(Env[i].var == var){
	    switch(Env[i].var_kind){
	    case VAR_ARG:
		genCodeI(LOADA,Env[i].pos);
		return;
	    case VAR_LOCAL:
		genCodeI(LOADL,Env[i].pos);
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
    for( ;params != NULL; params = getNext(params)){
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
	compileExpr(p);
	genCode(POP);
    }
}

void compileBlock(AST *local_vars,AST *statements)
{
    int v;
    int envp_save;

    envp_save = envp;
    for( ; local_vars != NULL;local_vars = getNext(local_vars)){
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
    compileExpr(expr);
    genCode(RET);
}

void compileCallFunc(Symbol *f,AST *args)
{
    int nargs;
    nargs = compileArgs(args);
    genCodeS(CALL,f->name);
    genCodeI(POPR,nargs);
}

int compileArgs(AST *args)
{
    int nargs;
    if(args != NULL){
	nargs = compileArgs(getNext(args));
	compileExpr(getFirst(args));
	return nargs+1;
    } else return 0;
}

void compileIf(AST *cond, AST *then_part, AST *else_part)
{
    int l1,l2;

    compileExpr(cond);
    l1 = label_counter++;
    genCodeI(BEQ0,l1);
    compileStatement(then_part);
    l2 = label_counter++;
    if(else_part != NULL){
	genCodeI(JUMP,l2);
	genCodeI(LABEL,l1);
	compileStatement(else_part);
	genCodeI(LABEL,l2);
    } else {
	genCodeI(LABEL,l1);
    }
}

void compileWhile(AST *cond,AST *body)
{
    int l1,l2;
    l1 = label_counter++;
    l2 = label_counter++;
    genCodeI(LABEL,l1);
    compileExpr(cond);
    genCodeI(BEQ0,l2);
    compileStatement(body);
    genCodeI(JUMP,l1);
    genCodeI(LABEL,l2);
}

void compileFor(AST *init,AST *cond,AST *iter,AST *body)
{
    /* not implemented */
}

