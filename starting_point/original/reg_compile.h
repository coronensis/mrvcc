
#define VAR_ARG 0
#define VAR_LOCAL 1

#define MAX_ENV 100

typedef struct env {
    Symbol *var;
    int var_kind;
    int pos;
} Environment;

extern int tmp_counter;

/* reg_compile_expr.c */
void compileExpr(int target, AST *p);

/* reg_compile.c */
void compileStoreVar(Symbol *var,int r);
void compileLoadVar(int target, Symbol *var);
void compileStatement(AST *p);
void compileBlock(AST *local_vars,AST *statements);
void compileReturn(AST *expr);
void compileCallFunc(int target, Symbol *f,AST *args);
void compileIf(AST *cond, AST *then_part, AST *else_part);
void compileWhile(AST *cond,AST *body);
void compileFor(AST *init,AST *cond,AST *iter,AST *body);


