#include <stdlib.h>
#include <stdio.h>
#include "AST.h"

main()
{
    yyparse();
    return 0;
}

void error(char *msg)
{
    fprintf(stderr,"compiler error: %s",msg);
    exit(1);
}


