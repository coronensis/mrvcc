/*
 * stack machine opcode
 */
#define POP	0
#define PUSHI 	1	/* push int */
#define ADD 	2
#define SUB 	3
#define MUL 	4
#define GT	5 
#define LT	6
#define BEQ0	7	/* branch if eq 0 */
#define LOADA 	8	/* load arg */
#define LOADL	9	/* load local var */
#define STOREA	10	/* store arg */
#define STOREL	11	/* store local var */
#define JUMP	12
#define CALL	13
#define RET	14	/* return */
#define POPR	15
#define FRAME 	16	/* setup frame */
#define PRINTLN	17	/* println function */
#define ENTRY	18
#define LABEL 	19	/* label */

char *st_code_name(int code);
int get_st_code(char *name);


