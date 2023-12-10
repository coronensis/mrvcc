#include <stdlib.h>
#include <stdio.h>
#include "st_code.h"

struct {
    char *name;
    int code;
} st_code_table[] = {
    {"POP", 0 },
    { "PUSHI", PUSHI},	/* push int */
    { "ADD", 	ADD},
    { "SUB", 	SUB},
    { "MUL", 	MUL},
    { "GT",	GT},
    { "LT",	LT},
    { "BEQ0",	BEQ0},	/* branch if eq 0 */
    { "LOADA", 	LOADA},	/* load arg */
    { "LOADL",	LOADL},	/* load local var */
    { "STOREA",	STOREA},	/* store arg */
    { "STOREL",	STOREL},	/* store local var */
    { "JUMP",	JUMP},
    { "CALL",	CALL},
    { "RET",	RET},	/* return */
    { "POPR",	POPR},
    { "FRAME",	FRAME},	/* setup frame */
    { "PRINTLN", PRINTLN},	/* println function */
    { "ENTRY",	ENTRY},
    { "LABEL", 	LABEL},	/* label */
    { 0,0}
};

char *st_code_name(int code)
{
    int i;
    for(i = 0; st_code_table[i].name != 0; i++)
	if(st_code_table[i].code == code) 
	    return st_code_table[i].name;
    fprintf(stderr,"unknown code = %d\n",code);
    exit(1);
}    

int get_st_code(char *name)
{
    int i;
    for(i = 0; st_code_table[i].name != 0; i++)
	if(strcmp(st_code_table[i].name,name) == 0) 
	    return st_code_table[i].code;
    fprintf(stderr,"unknown code name = %s\n",name);
    exit(1);
}






