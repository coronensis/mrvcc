#include <stdlib.h>
#include "AST.h"
#include "interp.h"

int main()
{
    int r;
    yyparse();

    /* execute main */
    printf("execute main ...\n");
    r = executeCallFunc(lookupSymbol("main"),NULL);
    printf("execute end ...\n");
    return r;
}

void error(char *msg)
{
    printf("ERROR: %s\n",msg);
    exit(1);
}
