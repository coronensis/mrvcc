#define MAX_ENV 100
#define MAX_CODE 200

#define VAR_ARG 0
#define VAR_LOCAL 1

typedef struct env {
    Symbol *var;
    int var_kind;
    int pos;
} Environment;

/* defined in st_compile.c */
void compileStoreVar(Symbol *var,AST *v);
void compileLoadVar(Symbol *var);
void compileStatement(AST *p);
void compileBlock(AST *local_vars,AST *statements);
void compileReturn(AST *expr);
void compileCallFunc(Symbol *f,AST *args);
int  compileArgs(AST *args);
void compileIf(AST *cond, AST *then_part, AST *else_part);
void compileWhile(AST *cond, AST *body);
void compileFor(AST *init,AST *cond,AST *iter,AST *body);

/* defined in st_compile_expr.c */
void  compileExpr(AST *p);

/* defined in st_code_gen.c */
void initGenCode(void);
void genCode(int opcode);
void genCodeI(int opcode, int i);
void genCodeS(int opcode,char *s);
void genFuncCode(char *entry_name, int n_local);
