#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define VAR_ARGUMENT     0
#define VAR_LOCAL        1

#define N_REG            4
#define N_SAVE           N_REG

#define TMP_OFF(i)	 -((i+1)+1)*4
#define LOCAL_VAR_OFF(i) -(N_SAVE+1+(i+1))*4
#define ARG_OFF(i)	 ((i)+2)*4

#define REG_AX           0
#define REG_DX           3

typedef enum {
	 OP_LOADI	/* r,i	load int */
	,OP_LOADA  	/* r,n	load arg */
	,OP_LOADL 	/* r,n	load local variable */
	,OP_STOREA	/* r,n	store arg */
	,OP_STOREL	/* r,n	store local variable */
	,OP_ADD 	/* t,r1,r2 */
	,OP_SUB 	/* t,r1,r2 */
	,OP_MUL		/* t,r1,r2 */
	,OP_DIV		/* t,r1,r2 */
	,OP_MOD		/* t,r1,r2 */
	,OP_BITWISE_AND	/* t,r1,r2 */
	,OP_LOGICAL_AND	/* t,r1,r2 */
	,OP_NOT		/* t,r1,r2 */
	,OP_BITWISE_OR	/* t,r1,r2 */
	,OP_LOGICAL_OR	/* t,r1,r2 */
	,OP_XOR		/* t,r1,r2 */
	,OP_SLL		/* t,r1,r2 */
	,OP_SRL		/* t,r1,r2 */
	,OP_LT		/* r,r1,r2 */
	,OP_GT		/* t,r1,r2 */
	,OP_LE		/* r,r1,r2 */
	,OP_GE		/* r,r1,r2 */
	,OP_EQ		/* r,r1,r2 */
	,OP_NE		/* r,r1,r2 */
	,OP_BEQZ	/* r,L branch if eq 0 */
	,OP_JUMP	/* L   */
	,OP_ARG 	/* r,n  set Argument */
	,OP_CALL	/* r,func */
	,OP_RET		/* r  return */
	,OP_LABEL 	/* L  label */
	,OP_LOAD
	,OP_STORE
	,OP_INC
	,OP_DEC
	,OP_UPLUS
	,OP_UMINUS
	,OP_LOGICAL_NOT
	,OP_BITWISE_NOT
	,OP_DEREF
	,OP_ADDR
} OpCodes;

typedef enum {
	 NODE_LIST
	,NODE_COMPOUND
	,NODE_NUMBER
	,NODE_STRING
	,NODE_IDENTIFIER
	,NODE_ASSIGNMENT
	,NODE_ADD
	,NODE_SUB
	,NODE_MUL
	,NODE_DIV
	,NODE_MOD
	,NODE_BITWISE_AND
	,NODE_LOGICAL_AND
	,NODE_BITWISE_OR
	,NODE_LOGICAL_OR
	,NODE_XOR
	,NODE_NOT
	,NODE_SLL
	,NODE_SRL
	,NODE_LT
	,NODE_GT
	,NODE_LE
	,NODE_GE
	,NODE_EQ
	,NODE_NE
	,NODE_IF
	,NODE_RETURN
	,NODE_WHILE
	,NODE_CALL
	,NODE_INC
	,NODE_DEC
	,NODE_UPLUS
	,NODE_UMINUS
	,NODE_LOGICAL_NOT
	,NODE_BITWISE_NOT
	,NODE_DEREF
	,NODE_ADDR
} NodeTypes;


typedef struct _Symbol {
    char *name;
    struct _Symbol *next;
} Symbol;

Symbol *symbol_table_head = NULL;
Symbol *symbol_table_tail = NULL;


typedef struct _ThreeAddressCode {
        int opcode;
        int operand_int_1;
        int operand_int_2;
        int operand_int_3;
        char *operand_str;
        struct _ThreeAddressCode *next;
} ThreeAddressCode;

ThreeAddressCode *intermediate_code_head = NULL;
ThreeAddressCode *intermediate_code_tail = NULL;


typedef struct _Variable {
        Symbol *sym;
        int type;
        int pos;
        struct _Variable *prev;
        struct _Variable *next;
} Variable;

Variable *variables_head = NULL;
Variable *variables_tail = NULL;
Variable *variables_current = NULL;


typedef struct Node {
        NodeTypes type;
        struct Node *left;
        struct Node *right;
        Symbol *identifier;
        char *string;
        int integer;
} ParseTreeNode;


char *register_tmp_names[N_REG] = { "t0", "t1", "t2", "t3" };
int register_tmp_state[N_REG];
int register_tmp_saved[N_SAVE];

int label_counter = 0;
int local_var_pos;
int register_counter = 0;

// -----------------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------------

int yylex();

void yyerror()
{
    printf("parser error. syntax invalid!\n");
    exit(1);
}

void error(char *msg)
{
    fprintf(stderr,"compiler error: %s",msg);
    exit(1);
}

void compile_statement(ParseTreeNode *node);
void compile_expression(int target, ParseTreeNode *node);

// -----------------------------------------------------------------------------
// symbol table
// -----------------------------------------------------------------------------

Symbol *symbol_find(const char *name)
{
    Symbol *found = NULL;
    if (symbol_table_head) {
        Symbol *sym = symbol_table_head;
        for (; sym; sym = sym->next)
            if (!strcmp(name, sym->name)) {
                found = sym;
                break;
            }
    }
    return found;
}

Symbol *identifier_find_or_create(const char *name)
{
    Symbol *sym = symbol_find(name);
    if (sym == NULL) {
        sym = (Symbol*)malloc(sizeof(Symbol));

        sym->name = strdup(name);
        sym->next = NULL;

        if (!symbol_table_head) {
            symbol_table_head = sym;
            symbol_table_tail = sym;
        }
        else {
            symbol_table_tail->next = sym;
            symbol_table_tail = sym;
        }
    }

    return sym;
}

// -----------------------------------------------------------------------------
// parse tree construction and navigation
// -----------------------------------------------------------------------------

ParseTreeNode *new_op_node(NodeTypes type, ParseTreeNode *left, ParseTreeNode *right)
{
	ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
	node->type = type;
	node->right = right;
	node->left = left;
	return node;
}

ParseTreeNode *new_identifier_node(char *name)
{
	ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
	node->type = NODE_IDENTIFIER;
	node->identifier = identifier_find_or_create(name);
	return node;
}

Symbol *identifier_get(ParseTreeNode *node)
{
	if (node->type != NODE_IDENTIFIER) {
		fprintf(stderr,"bad access to symbol\n");
		exit(1);
	}
	else
		return node->identifier;
}

ParseTreeNode *new_string_node(char *string)
{
	ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
	node->type = NODE_STRING;
	node->string = strdup(string);
	return node;
}

ParseTreeNode *new_number_node(char *value)
{
	ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
	node->type = NODE_NUMBER;
	node->integer = atoi(value);
	return node;
}

ParseTreeNode *get_list_node_nth_left(ParseTreeNode *node,int nth)
{
	if (node->type != NODE_LIST) {
		fprintf(stderr,"bad access to list\n");
		exit(1);
	}
	if(nth > 0)
		return get_list_node_nth_left(node->right, nth - 1);
	else
		return node->left;
}

ParseTreeNode *new_list_node_add_last(ParseTreeNode *l ,ParseTreeNode *node)
{
	ParseTreeNode *q;
	if (l == NULL)
		return new_op_node(NODE_LIST,node,NULL);
	q = l;
	while (q->right != NULL)
		q = q->right;
	q->right = new_op_node(NODE_LIST,node,NULL);
	return l;
}

ParseTreeNode *get_list_node_right(ParseTreeNode *node)
{
	if (node->type != NODE_LIST) {
		fprintf(stderr,"bad access to list\n");
		exit(1);
	}
	else
		return node->right;
}

char *node_type_name(NodeTypes type)
{
	switch(type) {
		case NODE_LIST:		return "NODE_LIST";
		case NODE_NUMBER:	return "NODE_NUMBER";
		case NODE_IDENTIFIER:	return "NODE_IDENTIFIER";
		case NODE_ASSIGNMENT:	return "NODE_ASSIGNMENT";
		case NODE_CALL:		return "NODE_CALL";
		case NODE_IF:		return "NODE_IF";
		case NODE_COMPOUND:	return "NODE_COMPOUND";
		case NODE_RETURN:	return "NODE_RETURN";
		case NODE_WHILE:	return "NODE_WHILE";
		case NODE_ADD:		return "NODE_ADD";
		case NODE_SUB:		return "NODE_SUB";
		case NODE_MUL:		return "NODE_MUL";
		case NODE_DIV:		return "NODE_DIS";
		case NODE_MOD:		return "NODE_MOD";
		case NODE_SLL:		return "NODE_SLL";
		case NODE_SRL:		return "NODE_SRL";
		case NODE_XOR:		return "NODE_XOR";
		case NODE_NOT:		return "NODE_NOT";
		case NODE_BITWISE_AND:	return "NODE_BITWISE_AND";
		case NODE_BITWISE_OR:	return "NODE_BITWISE_OR";
		case NODE_LOGICAL_AND:	return "NODE_LOGICAL_AND";
		case NODE_LOGICAL_OR:	return "NODE_LOGICAL_OR";
		case NODE_LT:		return "NODE_LT";
		case NODE_GT:		return "NODE_GT";
		case NODE_LE:		return "NODE_LE";
		case NODE_GE:		return "NODE_GE";
		case NODE_EQ:		return "NODE_EQ";
		case NODE_NE:		return "NODE_NE";
		case NODE_INC:		return "NODE_INC";
		case NODE_DEC:		return "NODE_DEC";
		case NODE_UPLUS:	return "NODE_UPLUS";
		case NODE_UMINUS:	return "NODE_UMINUS";
		case NODE_LOGICAL_NOT:	return "NODE_LOGICAL_NOT";
		case NODE_BITWISE_NOT:	return "NODE_BITWISE_NOT";
		case NODE_DEREF:	return "NODE_DEREF";
		case NODE_ADDR:		return "NODE_ADDR";
		default:		return "???";
	}
}

void print_parse_tree(ParseTreeNode *node)
{
	if(node == NULL) {
		fprintf(stderr, "(-end-)\n");
		return;
	}
	switch(node->type) {
		case NODE_NUMBER:
			fprintf(stderr,"%d",node->integer);
			break;
		case NODE_IDENTIFIER:
			fprintf(stderr, "'%s'",node->identifier->name);
			break;
		case NODE_LIST:
			fprintf(stderr, "\n(LIST ");
			while(node != NULL){
				print_parse_tree(node->left);
				node = node->right;
				if(node != NULL) {
					fprintf(stderr, " ");
				}
			}
			fprintf(stderr, ")\n");
			break;
		default:
			fprintf(stderr, "\n(%s ",node_type_name(node->type));
			print_parse_tree(node->left);
			fprintf(stderr, " ");
			print_parse_tree(node->right);
			fprintf(stderr, ")\n");
	}
        fflush(stdout);
}

// -----------------------------------------------------------------------------
// register allocation
// -----------------------------------------------------------------------------

void register_tmp_init()
{
	int i;
	for(i = 0; i < N_REG; i++)
		register_tmp_state[i] = -1;
	for(i = 0; i < N_SAVE; i++)
		register_tmp_saved[i] = -1;
}

int register_get_free(int reg)
{
	int i;
	for(i = 0; i < N_REG; i++)
		if(register_tmp_state[i] < 0){
			register_tmp_state[i] = reg;
			return i;
		}
	error("no free temp reg");
	return -1;
}

int register_use(int reg)
{
	int i,rr;
	for(i = 0; i < N_REG; i++)
		if (register_tmp_state[i] == reg)
			return i;
	/* not found in register, then restore from save area. */
	for(i = 0; i < N_SAVE; i++)
		if (register_tmp_saved[i] == reg) {
			rr = register_get_free(reg);
			register_tmp_saved[i] = -1;
			// Load into register
			printf("\tlw\t%s,%d(fp)\n", register_tmp_names[rr], TMP_OFF(i));
			return rr;
		}
	error("reg is not found");
	return -1;
}

void register_save(int reg)
{
	int i;
	if (register_tmp_state[reg] < 0)
		return;
	for (i = 0; i < N_SAVE; i++)
		if (register_tmp_saved[i] < 0) {
			// Save register
			printf("\tsw\t%s,%d(fp)\n", register_tmp_names[reg], TMP_OFF(reg));
			register_tmp_saved[i] = register_tmp_state[reg];
			register_tmp_state[reg] = -1;
			return;
		}
	error("no free temp save");
}

void register_save_all()
{
	int i;
	for (i = 0; i < N_REG; i++)
		register_save(i);
}

void register_assign(int reg, int value)
{
	if (register_tmp_state[reg] == value)
		return;
	register_save(reg);
	register_tmp_state[reg] = value;
}

void register_free(int reg)
{
	register_tmp_state[reg] = -1;
}

// -----------------------------------------------------------------------------
// intermediate code generation
// -----------------------------------------------------------------------------

ThreeAddressCode *intermediate_code_new(void)
{
	ThreeAddressCode *code = (ThreeAddressCode*)malloc(sizeof(ThreeAddressCode));
	memset(code, 0, sizeof(ThreeAddressCode));
	return code;
}

void intermediate_code_add(ThreeAddressCode *code)
{
	if (intermediate_code_head == NULL) {
		intermediate_code_head = code;
		intermediate_code_tail = code;
	}
	else {
		intermediate_code_tail->next = code;
		intermediate_code_tail = code;
	}
}

void generate_im_code_one_operand(int opcode, int operand_int_1)
{
	ThreeAddressCode *code = intermediate_code_new();
	code->opcode = opcode;
	code->operand_int_1 = operand_int_1;
	intermediate_code_add(code);
}

void generate_im_code_two_operand(int opcode, int operand_int_1, int operand_int_2)
{
	ThreeAddressCode *code = intermediate_code_new();
	code->opcode = opcode;
	code->operand_int_1 = operand_int_1;
	code->operand_int_2 = operand_int_2;
	intermediate_code_add(code);
}

void generate_im_code_three_operand(int opcode, int operand_int_1, int operand_int_2, int operand_int_3)
{
	ThreeAddressCode *code = intermediate_code_new();
	code->opcode = opcode;
	code->operand_int_1 = operand_int_1;
	code->operand_int_2 = operand_int_2;
	code->operand_int_3 = operand_int_3;
	intermediate_code_add(code);
}

void generate_im_code_two_operand_and_string(int opcode, int operand_int_1, int operand_int_2, char *s)
{
	ThreeAddressCode *code = intermediate_code_new();
	code->opcode = opcode;
	code->operand_int_1 = operand_int_1;
	code->operand_int_2 = operand_int_2;
	code->operand_str = s;
	intermediate_code_add(code);
}

int generate_string(char *s)
{
	int l = label_counter++;
	printf("\t.section\t.rodata\n");
	printf(".LC%d:\n",l);
	printf("\t.string %s\n",s);
	return l;
}

void emit_function_code(char *entry_name, int n_local)
{
	int operand_int_1, operand_int_2, operand_int_3;
	int reg, reg1, reg2;
	char *operand_str;
	int ret_lab, label1, label2;
	int frame_size;

	/* function header */
	puts("\t.text");       	           				/* .text         			 */
	puts("\t.align\t2");        	   				/* .align 2      			 */
	printf("\t.globl\t%s\n", entry_name);    		/* .globl <name>			 */
	printf("\t.type\t%s,@function\n", entry_name);	/* .type <name>,@function	 */
	printf("%s:\n", entry_name);             		/* <name>:					 */

	// Function prologue
	frame_size = -LOCAL_VAR_OFF(n_local);
	ret_lab = label_counter++;
	//addi    sp,sp,-64
	printf("\taddi\tsp,sp,%d\n",-frame_size);
	printf("\tsw\tra,%d(sp)\n", frame_size-4);
	printf("\tsw\tfp,%d(sp)\n", frame_size-8);
	printf("\taddi\tfp,sp,%d\n", frame_size);

	register_tmp_init();

	ThreeAddressCode *code = NULL;
	for (code = intermediate_code_head; code != NULL; code = code->next) {
		operand_int_1 = code->operand_int_1;
		operand_int_2 = code->operand_int_2;
		operand_int_3 = code->operand_int_3;
		operand_str = code->operand_str;

		switch(code->opcode){
		case OP_LOADI:
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			printf("\tli\t%s,%d\n", register_tmp_names[reg], operand_int_2);
			break;
		case OP_LOADA:	/* load arg */
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			printf("\tlw\t%s,%d(fp)\n", register_tmp_names[reg], ARG_OFF(operand_int_2));
			break;
		case OP_LOADL:	/* load local */
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			printf("\tlw\t%s,%d(fp)\n", register_tmp_names[reg],LOCAL_VAR_OFF(operand_int_2));
			break;
		case OP_STOREA:	/* store arg */
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tsw\t%s,%d(fp)\n", register_tmp_names[reg], ARG_OFF(operand_int_2));
			break;
		case OP_STOREL:	/* store local */
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tsw\t%s,%d(fp)\n", register_tmp_names[reg],LOCAL_VAR_OFF(operand_int_2));
			break;
		case OP_BEQZ:	/* conditional branch */
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tbeq\t%s,zero,.L%d\n",register_tmp_names[reg], operand_int_2);
			break;
		case OP_LABEL:
			printf(".L%d:\n",operand_int_1);
			break;
		case OP_JUMP:
			printf("\tj\t.L%d\n",operand_int_1);
			break;
		case OP_CALL:
			register_save_all();
			printf("\tcall\t%s\n",operand_str);
			if(operand_int_1 < 0)
				break;
			register_assign(REG_AX, operand_int_1);
//FIXME: Needed?			printf("\tadd $%d,%%esp\n",operand_int_2 * 4);
			break;
		case OP_ARG:
			reg = register_use(operand_int_1);
			register_free(reg);
//			printf("\tmv\ta%d,%s\n", n_arg, register_tmp_names[reg]);
			break;
		case OP_RET:
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tmv\ta0,%s\n",register_tmp_names[reg]);
			printf("\tj\t.L%d\n",ret_lab);
			break;
		case OP_ADD:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\tadd\t%s,%s,%s\n",register_tmp_names[reg1],register_tmp_names[reg1],register_tmp_names[reg2]);
			break;
		case OP_SUB:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\tsub\t%s,%s,%s\n",register_tmp_names[reg1],register_tmp_names[reg1],register_tmp_names[reg2]);
			break;
		case OP_MUL:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(REG_AX, operand_int_1);
			register_save(REG_DX);
			if(reg1 != REG_AX)
				printf("\taddi %s,%s,0\n",register_tmp_names[REG_AX],register_tmp_names[reg1]);
			printf("\tmul\t%s,%s,%s\n",register_tmp_names[REG_AX],register_tmp_names[REG_AX],register_tmp_names[reg2]);
			break;
		case OP_DIV:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(REG_AX, operand_int_1);
			register_save(REG_DX);
			if(reg1 != REG_AX)
				printf("\taddi %s,%s,0\n",register_tmp_names[REG_AX],register_tmp_names[reg1]);
			printf("\tdiv\t%s,%s,%s\n",register_tmp_names[REG_AX],register_tmp_names[REG_AX],register_tmp_names[reg2]);
			break;
		case OP_MOD:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(REG_AX, operand_int_1);
			register_save(REG_DX);
			if(reg1 != REG_AX)
				printf("\taddi %s,%s,0\n",register_tmp_names[REG_AX],register_tmp_names[reg1]);
			printf("\trem\t%s,%s,%s\n",register_tmp_names[REG_AX],register_tmp_names[REG_AX],register_tmp_names[reg2]);
			break;
		case OP_SLL:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\tsll\t%s,%s,%s\n",register_tmp_names[reg1],register_tmp_names[reg1],register_tmp_names[reg2]);
			break;
		case OP_SRL:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\tsrl\t%s,%s,%s\n",register_tmp_names[reg1],register_tmp_names[reg1],register_tmp_names[reg2]);
			break;
		case OP_BITWISE_AND:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\tand\t%s,%s,%s\n",register_tmp_names[reg1],register_tmp_names[reg1],register_tmp_names[reg2]);
			break;
		case OP_LOGICAL_AND:
			// TODO
			break;
		case OP_NOT:
			// TODO
			break;
		case OP_BITWISE_OR:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\tor\t%s,%s,%s\n",register_tmp_names[reg1],register_tmp_names[reg1],register_tmp_names[reg2]);
			break;
		case OP_LOGICAL_OR:
			// TODO
			break;
		case OP_XOR:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\txor\t%s,%s,%s\n",register_tmp_names[reg1],register_tmp_names[reg1],register_tmp_names[reg2]);
			break;
		case OP_LT:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			label1 = label_counter++;
			label2 = label_counter++;
			printf("\tblt\t%s,%s,.L%d\n",register_tmp_names[reg1],register_tmp_names[reg2], label1);
			printf("\taddi\t%s,zero,0\n",register_tmp_names[reg]);
			printf("\tj\t.L%d\n",label2);
			printf(".L%d:\taddi\t%s,zero,1\n",label1,register_tmp_names[reg]);
			printf(".L%d:",label2);
			break;
		case OP_GT:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			label1 = label_counter++;
			label2 = label_counter++;
			printf("\tbgt\t%s,%s,.L%d\n",register_tmp_names[reg1],register_tmp_names[reg2], label1);
			printf("\taddi\t%s,zero,0\n",register_tmp_names[reg]);
			printf("\tj\t.L%d\n",label2);
			printf(".L%d:\taddi\t%s,zero,1\n",label1,register_tmp_names[reg]);
			printf(".L%d:",label2);
			break;
		case OP_LE:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			label1 = label_counter++;
			label2 = label_counter++;
			printf("\tble\t%s,%s,.L%d\n",register_tmp_names[reg1],register_tmp_names[reg2], label1);
			printf("\taddi\t%s,zero,0\n",register_tmp_names[reg]);
			printf("\tj\t.L%d\n",label2);
			printf(".L%d:\taddi\t%s,zero,1\n",label1,register_tmp_names[reg]);
			printf(".L%d:",label2);
			break;
		case OP_GE:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			label1 = label_counter++;
			label2 = label_counter++;
			printf("\tbge\t%s,%s,.L%d\n",register_tmp_names[reg1],register_tmp_names[reg2], label1);
			printf("\taddi\t%s,zero,0\n",register_tmp_names[reg]);
			printf("\tj\t.L%d\n",label2);
			printf(".L%d:\taddi\t%s,zero,1\n",label1,register_tmp_names[reg]);
			printf(".L%d:",label2);
			break;
		case OP_EQ:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			label1 = label_counter++;
			label2 = label_counter++;
			printf("\tbeq\t%s,%s,.L%d\n",register_tmp_names[reg1],register_tmp_names[reg2], label1);
			printf("\taddi\t%s,zero,0\n",register_tmp_names[reg]);
			printf("\tj\t.L%d\n",label2);
			printf(".L%d:\taddi\t%s,zero,1\n",label1,register_tmp_names[reg]);
			printf(".L%d:",label2);
			break;
		case OP_NE:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			label1 = label_counter++;
			label2 = label_counter++;
			printf("\tbne\t%s,%s,.L%d\n",register_tmp_names[reg1],register_tmp_names[reg2], label1);
			printf("\taddi\t%s,zero,0\n",register_tmp_names[reg]);
			printf("\tj\t.L%d\n",label2);
			printf(".L%d:\taddi\t%s,zero,1\n",label1,register_tmp_names[reg]);
			printf(".L%d:",label2);
			break;
		case OP_INC:
			reg = register_use(operand_int_1);
			register_assign(reg, operand_int_1);
			printf("\taddi\t%s,%s,1\n", register_tmp_names[reg], register_tmp_names[reg]);
			register_free(reg);
			break;
		case OP_DEC:
			reg = register_use(operand_int_1);
			register_assign(reg, operand_int_1);
			printf("\taddi\t%s,%s,-1\n", register_tmp_names[reg], register_tmp_names[reg]);
			register_free(reg);
			break;
		case OP_UPLUS:
			// TODO
			break;
		case OP_UMINUS:
			// TODO
			break;
		case OP_LOGICAL_NOT:
			// TODO
			break;
		case OP_BITWISE_NOT:
			// TODO
			break;
		case OP_DEREF:
			// TODO
			break;
		case OP_ADDR:
			// TODO
			break;
		}
	}
	// Reset. Leaks memory but we don't care...
	intermediate_code_head = NULL;
	intermediate_code_tail = NULL;

	// Function epilogue
	printf(".L%d:\tlw\tra,%d(sp)\n",ret_lab, frame_size-4);
	printf("\tlw\tfp,%d(sp)\n", frame_size-8);
	printf("\taddi\tsp,sp,%d\n", frame_size);
	printf("\tjr\tra\n");
}

// -----------------------------------------------------------------------------
// compile
// -----------------------------------------------------------------------------

Variable *variable_new(void)
{
	Variable *var = (Variable*)malloc(sizeof(Variable));
	memset(var, 0, sizeof(Variable));
	return var;
}

void variable_add(Variable *var)
{
	if (variables_head == NULL) {
		variables_head = var;
		variables_tail = var;
	}
	else {
		var->prev = variables_tail;
		variables_tail->next = var;
		variables_tail = var;
	}
}

void compile_compound_statement(ParseTreeNode *local_vars, ParseTreeNode *statements)
{
	Variable *variables_current_saved = variables_current;
	for (; local_vars != NULL; local_vars = get_list_node_right(local_vars)) {
		Variable *var = variable_new();
		var->sym = identifier_get(get_list_node_nth_left(local_vars, 0));
		var->type = VAR_LOCAL;
		var->pos = local_var_pos++;
		variable_add(var);
		variables_current = var;
	}
	for (;statements != NULL; statements = get_list_node_right(statements)) {
		compile_statement(get_list_node_nth_left(statements,0));
	}
	variables_current = variables_current_saved;
}

int compile_function_arguments(ParseTreeNode *args, int i)
{
	int n;
	if (args != NULL) {
		n = compile_function_arguments(get_list_node_right(args), i + 1);
		int r = register_counter++;
		compile_expression(r,get_list_node_nth_left(args, 0));
		generate_im_code_two_operand(OP_ARG,r,i);
	}
	else
		return 0;
	return n + 1;
}

void compile_function_call(int target, Symbol *f, ParseTreeNode *args)
{
	int narg;
	narg = compile_function_arguments(args,0);
	generate_im_code_two_operand_and_string(OP_CALL, target, narg, f->name);
}

void compile_load_store_variable(Symbol *sym, int target_reg, int operation)
{
	Variable *var = NULL;
	for (var = variables_current; var; var = var->prev) {
		if (var->sym == sym) {
			switch (var->type) {
			case VAR_ARGUMENT:
				if (operation == OP_LOAD)
					generate_im_code_two_operand(OP_LOADA, target_reg, var->pos);
				else
					generate_im_code_two_operand(OP_STOREA, target_reg, var->pos);
				return;
			case VAR_LOCAL:
				if (operation == OP_LOAD)
					generate_im_code_two_operand(OP_LOADL, target_reg, var->pos);
				else
					generate_im_code_two_operand(OP_STOREL, target_reg, var->pos);
				return;
			}
		}
	}
	error("undefined variable\n");
}

void compile_expression(int target, ParseTreeNode *node)
{
	int reg1,reg2;
	if (node == NULL)
		return;
	switch(node->type) {
	case NODE_NUMBER:
		generate_im_code_two_operand(OP_LOADI,target,node->integer);
		return;
	case NODE_IDENTIFIER:
		compile_load_store_variable(identifier_get(node), target, OP_LOAD);
		return;
	case NODE_ASSIGNMENT:
		if(target != -1)
			error("assign has no value");
		reg1 = register_counter++;
		compile_expression(reg1,node->right);
		compile_load_store_variable(identifier_get(node->left),reg1, OP_STORE);
		return;
	case NODE_ADD:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_ADD,target,reg1,reg2);
		return;
	case NODE_SUB:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_SUB,target,reg1,reg2);
		return;
	case NODE_MUL:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_MUL,target,reg1,reg2);
		return;
	case NODE_DIV:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_DIV,target,reg1,reg2);
		return;
	case NODE_MOD:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_DIV,target,reg1,reg2);
		return;
	case NODE_LT:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_LT,target,reg1,reg2);
		return;
	case NODE_GT:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_GT,target,reg1,reg2);
		return;
	case NODE_LE:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_LE,target,reg1,reg2);
		return;
	case NODE_GE:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_GE,target,reg1,reg2);
		return;
	case NODE_EQ:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_EQ,target,reg1,reg2);
		return;
	case NODE_NE:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_NE,target,reg1,reg2);
		return;
	case NODE_CALL:
		compile_function_call(target, identifier_get(node->left), node->right);
		return;
	case NODE_BITWISE_AND:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_BITWISE_AND,target,reg1,reg2);
		return;
	case NODE_LOGICAL_AND:
		// TODO
		return;
	case NODE_BITWISE_OR:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_BITWISE_OR,target,reg1,reg2);
		return;
	case NODE_LOGICAL_OR:
		// TODO
		return;
	case NODE_XOR:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_XOR,target,reg1,reg2);
		return;
	case NODE_NOT:
		// TODO
		return;
	case NODE_SLL:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_SLL,target,reg1,reg2);
		return;
	case NODE_SRL:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_SRL,target,reg1,reg2);
		return;
	case NODE_INC:
		reg1 = register_counter++;
		compile_expression(reg1,node->left);
		generate_im_code_one_operand(OP_INC,reg1);
		return;
	case NODE_DEC:
		reg1 = register_counter++;
		compile_expression(reg1,node->left);
		generate_im_code_one_operand(OP_DEC,reg1);
		return;
	case NODE_UPLUS:
		// TODO
		return;
	case NODE_UMINUS:
		// TODO
		return;
	case NODE_LOGICAL_NOT:
		// TODO
		return;
	case NODE_BITWISE_NOT:
		reg1 = register_counter++;
		compile_expression(reg1,node->left);
		generate_im_code_one_operand(OP_BITWISE_NOT, reg1);
		return;
	case NODE_DEREF:
		// TODO
		return;
	case NODE_ADDR:
		// TODO
		return;
	default:
		error("unknown operator/statement\n");
	}
}

void compile_return_statement(ParseTreeNode *expr)
{
	int r;
	if(expr != NULL) {
		r = register_counter++;
		compile_expression(r,expr);
	}
	else
		r = -1;
	generate_im_code_one_operand(OP_RET,r);
}

void compile_if_statement(ParseTreeNode *cond, ParseTreeNode *then_part, ParseTreeNode *else_part)
{
	int label1,label2;
	int r = register_counter++;
	compile_expression(r,cond);
	label1 = label_counter++;
	generate_im_code_two_operand(OP_BEQZ,r,label1);
	compile_statement(then_part);
	if (else_part != NULL) {
		label2 = label_counter++;
		generate_im_code_one_operand(OP_JUMP,label2);
		generate_im_code_one_operand(OP_LABEL,label1);
		compile_statement(else_part);
		generate_im_code_one_operand(OP_LABEL,label2);
	}
	else
		generate_im_code_one_operand(OP_LABEL,label1);
}

void compile_while_statement(ParseTreeNode *cond,ParseTreeNode *body)
{
	int label1 = label_counter++;
	int label2 = label_counter++;
	int r = register_counter++;

	generate_im_code_one_operand(OP_LABEL,label1);
	compile_expression(r,cond);
	generate_im_code_two_operand(OP_BEQZ,r,label2);
	compile_statement(body);
	generate_im_code_one_operand(OP_JUMP,label1);
	generate_im_code_one_operand(OP_LABEL,label2);
}

void compile_statement(ParseTreeNode *node)
{
	if (node == NULL)
		return;
	switch (node->type) {
	case NODE_COMPOUND:
		compile_compound_statement(node->left, node->right);
		break;
	case NODE_RETURN:
		compile_return_statement(node->left);
		break;
	case NODE_IF:
		compile_if_statement(node->left, get_list_node_nth_left(node->right,0), get_list_node_nth_left(node->right,1));
		break;
	case NODE_WHILE:
		compile_while_statement(node->left, node->right);
		break;
	default:
		compile_expression(-1,node);
		break;
	}
}

void compile_function(Symbol *fsym, ParseTreeNode *params, ParseTreeNode *body)
{
	int param_pos;
	// init code generator
	variables_current = NULL;
	param_pos = 0;
	local_var_pos = 0;
	for (; params != NULL; params = get_list_node_right(params)) {
		Variable *var = variable_new();
		var->sym = identifier_get(get_list_node_nth_left(params,0));
		var->type = VAR_ARGUMENT;
		var->pos = param_pos++;
		variable_add(var);
		variables_current = var;
	}
	//print_parse_tree(body);
	compile_statement(body);
	emit_function_code(fsym->name, local_var_pos);
	variables_current = NULL;
}

