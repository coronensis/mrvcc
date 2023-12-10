#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include "st_code.h"

/* 
 * virtual stack machine for tiny C
 */
#define MAX_CODE 100
#define MAX_LABEL 20
#define MAX_STACK 200

struct code {
    int opcode;
    int i_operand;
    char *s_operand;
} Codes[MAX_CODE];

struct label {
    char *name;
    int addr;
} Labels[MAX_LABEL];

int n_codes,n_labels;
int findLabel(char *name);
void readCode(void);
void executeCode(int start_pc);

int main()
{
    readCode();
    executeCode(findLabel("main"));
}

char line[100];

void readCode()
{
    char opcode_name[10],buf[100];
    int opcode,i;

    n_codes = 0;
    n_labels = 0;
    while(fgets(line,100,stdin) != NULL){
	if(sscanf(line,"%s",opcode_name) != 1) goto err;
	opcode = get_st_code(opcode_name);
	Codes[n_codes].opcode = opcode;
	switch(opcode){
	case PUSHI:
	case LOADA:
	case LOADL:
	case STOREA:
	case STOREL:
	case FRAME:
	case POPR:
	    if(sscanf(line,"%s %d",opcode_name,&i) != 2) goto err;
	    Codes[n_codes].i_operand = i;
	    break;
	case BEQ0:
	case LABEL:
	case JUMP:
	case CALL:
	case ENTRY:
	    if(sscanf(line,"%s %s",opcode_name,buf) != 2) goto err;
	    if(opcode == LABEL || opcode == ENTRY){
		Labels[n_labels].name = strdup(buf);
		Labels[n_labels].addr = n_codes;
	    } else {
		Codes[n_codes].s_operand = strdup(buf);
	    }
	    break;
	case PRINTLN:
	{ char *p,*q;
	    for(p = line; *p != '\"'; p++) /* */;
	    q = buf;
	    for(p++; *p != '\"'; p++) *q++ = *p;
	    *q = '\0';
	    Codes[n_codes].s_operand = strdup(buf);
	    break;
	}
	}
	if(opcode == LABEL || opcode == ENTRY) n_labels++;
	else n_codes++;
    }

    /* fix labels */
    for(i = 0; i < n_codes; i++){
	switch(Codes[i].opcode){
	case BEQ0:
	case JUMP:
	case CALL:
	    Codes[i].i_operand = findLabel(Codes[i].s_operand);
	}
    }
    return;
err:
    fprintf(stderr,"code read error\n");
    exit(1);
}
	
int findLabel(char *name)
{
    int j;
    for(j = 0; j < n_labels; j++){
	if(strcmp(Labels[j].name,name) == 0)
	    return Labels[j].addr;
    }
    fprintf(stderr,"label '%s' is undefined\n",name);
    exit(1);
}

int Sp,Fp;
int Pc;

int Stack[MAX_STACK];
#define Push(x)	 Stack[Sp--] = (x)
#define Pop	 Stack[++Sp]
#define Top 	 Stack[Sp+1]

void executeCode(int start_pc)
{
    int x,y;

    Sp = Fp = MAX_STACK;
    Pc = start_pc;

next:
    /* debug *//* printf("%d: %s %d\n",Pc,st_code_name(Codes[Pc].opcode),
       Codes[Pc].i_operand); */
    switch(Codes[Pc].opcode){
    case POP:
	Pop; break;
    case PUSHI:
	Push(Codes[Pc].i_operand); break;
    case ADD:
	y = Pop; x = Pop; Push(x + y); break;
    case SUB:
	y = Pop; x = Pop; Push(x - y); break;
    case MUL:
	y = Pop; x = Pop; Push(x * y); break;
    case GT:
	y = Pop; x = Pop; Push(x > y); break;
    case LT:
	y = Pop; x = Pop; Push(x < y); break;
    case BEQ0:
	x = Pop; if(x == 0){ Pc = Codes[Pc].i_operand; goto next; }; break;
    case LOADA:
	Push(Stack[Fp+Codes[Pc].i_operand+3]); break;
    case LOADL:
	Push(Stack[Fp-Codes[Pc].i_operand]); break;
    case STOREA:
	Stack[Fp+Codes[Pc].i_operand+3] = Top; break;
    case STOREL:
	Stack[Fp-Codes[Pc].i_operand] = Top; break;
    case JUMP:
	Pc = Codes[Pc].i_operand; goto next;
    case CALL:
	Push(Pc+1); Pc = Codes[Pc].i_operand; goto next;
    case RET:
	x = Pop; 
	Sp = Fp; Fp = Pop; 
	if(Sp >= MAX_STACK){
	    printf("stop ...\n");
	    return;
	}
	Pc = Pop; goto next;
    case POPR:
	Sp = Sp + Codes[Pc].i_operand; Push(x); break;
    case FRAME:
	Push(Fp); Fp = Sp; Sp -= Codes[Pc].i_operand; break;
    case PRINTLN:
	printf(Codes[Pc].s_operand,Top); printf("\n"); break;
    }
    Pc++; goto next;
}





