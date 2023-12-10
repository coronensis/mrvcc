#include "AST.h"
#include "st_code.h"
#include "st_compile.h"

struct _code {
    int opcode;
    int operand;
    char *soperand;
} Codes[MAX_CODE];

int n_code;

void initGenCode()
{
    n_code = 0;
}

void genCode(int opcode)
{
    Codes[n_code++].opcode = opcode;
}

void genCodeI(int opcode, int i)
{
    Codes[n_code].opcode = opcode;
    Codes[n_code++].operand = i;
}

void genCodeS(int opcode,char *s)
{
    Codes[n_code].opcode = opcode;
    Codes[n_code++].soperand = s;
}

void genFuncCode(char *entry_name, int n_local)
{
    int i;

    printf("ENTRY %s\n",entry_name);
    printf("FRAME %d\n",n_local);
    for(i = 0; i < n_code; i++){
	printf("%s ",st_code_name(Codes[i].opcode));
	switch(Codes[i].opcode){
	case PUSHI:
	case LOADA:
	case LOADL:
	case STOREA:
	case STOREL:
	case POPR:
	    printf("%d",Codes[i].operand);
	    break;
	case BEQ0:
	case LABEL:
	case JUMP:
	    printf("L%d",Codes[i].operand);
	    break;
	case CALL:
	    printf("%s",Codes[i].soperand);
	    break;
	case PRINTLN:
	    printf("\"%s\"",Codes[i].soperand);
	    break;
	}
	printf("\n");
    }
    printf("RET\n");
}




