#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define VAR_ARGUMENT   0
#define VAR_LOCAL 1

#define N_REG     4
#define N_SAVE    N_REG

#define TMP_OFF(i)		 	-((i+1)+1)*4
#define LOCAL_VAR_OFF(i) 	-(N_SAVE+1+(i+1))*4
#define ARG_OFF(i)			((i)+2)*4

#define REG_AX    0
#define REG_DX    3

typedef enum {
	 OP_LOADI		/* r,i	load int */
	,OP_LOADA 	 	/* r,n	load arg */
	,OP_LOADL	 	/* r,n	load local variable */
	,OP_STOREA		/* r,n	store arg */
	,OP_STOREL		/* r,n	store local variable */
	,OP_ADD 		/* t,r1,r2 */
	,OP_SUB 		/* t,r1,r2 */
	,OP_MUL			/* t,r1,r2 */
	,OP_GT			/* t,r1,r2 */
	,OP_LT			/* r,r1,r2 */
	,OP_BEQ0		/* r,L branch if eq 0 */
	,OP_JUMP		/* L   */
	,OP_ARG 		/* r,n  set Argument */
	,OP_CALL		/* r,func */
	,OP_RET		/* r  return */
	,OP_PRINT		/* r,L println function */
	,OP_LABEL	 	/* L  label */
	,OP_LOAD
	,OP_STORE
} OpCodes;

typedef enum {
	 NODE_LIST
	,NODE_COMPOUND
	,NODE_NUMBER
	,NODE_STRING
	,NODE_IDENTIFIER
	,NODE_ASSIGNMENT
	,NODE_ADDITION
	,NODE_SUBTRACTION
	,NODE_MULTIPLICATION
	,NODE_LESS_THAN
	,NODE_GREATER_THAN
	,NODE_IF
	,NODE_RETURN
	,NODE_WHILE
	,NODE_FUNCTION_CALL
	,NODE_PRINTLN_CALL
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


char *register_tmp_names[N_REG] = { "%eax", "%ebx", "%ecx", "%edx" };
int register_tmp_state[N_REG];
int register_tmp_saved[N_SAVE];

int label_counter = 0;
int local_var_pos;
int register_counter = 0;

// -----------------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------------

int yylex();

void yyerror() {
    printf("parser error. syntax invalid!\n");
    exit(1);
}

void error(char *msg) {
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

ParseTreeNode *new_op_node(NodeTypes type, ParseTreeNode *left, ParseTreeNode *right) {
	ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
	node->type = type;
	node->right = right;
	node->left = left;
	return node;
}

ParseTreeNode *new_identifier_node(char *name) {
	ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
	node->type = NODE_IDENTIFIER;
	node->identifier = identifier_find_or_create(name);
	return node;
}

Symbol *identifier_get(ParseTreeNode *node) {
	if (node->type != NODE_IDENTIFIER) {
		fprintf(stderr,"bad access to symbol\n");
		exit(1);
	}
	else
		return node->identifier;
}

ParseTreeNode *new_string_node(char *string) {
	ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
	node->type = NODE_STRING;
	node->string = strdup(string);
	return node;
}

ParseTreeNode *new_number_node(char *value) {
	ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
	node->type = NODE_NUMBER;
	node->integer = atoi(value);
	return node;
}

ParseTreeNode *get_list_node_nth_left(ParseTreeNode *node,int nth) {
	if (node->type != NODE_LIST) {
		fprintf(stderr,"bad access to list\n");
		exit(1);
	}
	if(nth > 0)
		return get_list_node_nth_left(node->right, nth - 1);
	else
		return node->left;
}

ParseTreeNode *new_list_node_add_last(ParseTreeNode *l ,ParseTreeNode *node) {
	ParseTreeNode *q;
	if (l == NULL)
		return new_op_node(NODE_LIST,node,NULL);
	q = l;
	while (q->right != NULL)
		q = q->right;
	q->right = new_op_node(NODE_LIST,node,NULL);
	return l;
}

ParseTreeNode *get_list_node_right(ParseTreeNode *node) {
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
		case NODE_LIST:			return "NODE_LIST";
		case NODE_NUMBER:		return "NODE_NUMBER";
		case NODE_IDENTIFIER:		return "NODE_IDENTIFIER";
		case NODE_ASSIGNMENT:		return "NODE_ASSIGNMENT";
		case NODE_ADDITION:		return "NODE_ADDITION";
		case NODE_SUBTRACTION:		return "NODE_SUBTRACTION";
		case NODE_MULTIPLICATION:	return "NODE_MULTIPLICATION";
		case NODE_LESS_THAN:		return "NODE_LESS_THAN";
		case NODE_GREATER_THAN:		return "NODE_GREATER_THAN";
		case NODE_FUNCTION_CALL:	return "NODE_FUNCTION_CALL";
		case NODE_PRINTLN_CALL:		return "NODE_PRINTLN_CALL";
		case NODE_IF:			return "NODE_IF";
		case NODE_COMPOUND:		return "NODE_COMPOUND";
		case NODE_RETURN:		return "NODE_RETURN";
		case NODE_WHILE:		return "NODE_WHILE";
		default:			return "???";
	}
}

void print_parse_tree(ParseTreeNode *node) {
	if(node == NULL) {
		fprintf(stderr, "(-end-)");
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
			fprintf(stderr, "(LIST ");
			while(node != NULL){
				print_parse_tree(node->left);
				node = node->right;
				if(node != NULL)
					fprintf(stderr, " ");
			}
			fprintf(stderr, ")");
			break;
		default:
			fprintf(stderr, "(%s ",node_type_name(node->type));
			print_parse_tree(node->left);
			fprintf(stderr, " ");
			print_parse_tree(node->right);
			fprintf(stderr, ")");
	}
    fflush(stdout);
}

// -----------------------------------------------------------------------------
// register allocation
// -----------------------------------------------------------------------------

void register_tmp_init() {
	int i;
	for(i = 0; i < N_REG; i++)
		register_tmp_state[i] = -1;
	for(i = 0; i < N_SAVE; i++)
		register_tmp_saved[i] = -1;
}

int register_get_free(int reg) {
	int i;
	for(i = 0; i < N_REG; i++)
		if(register_tmp_state[i] < 0){
			register_tmp_state[i] = reg;
			return i;
		}
	error("no free temp reg");
	return -1;
}

int register_use(int reg) {
	int i,rr;
	for(i = 0; i < N_REG; i++)
		if (register_tmp_state[i] == reg)
			return i;
	/* not found in register, then restore from save area. */
	for(i = 0; i < N_SAVE; i++)
		if (register_tmp_saved[i] == reg) {
			rr = register_get_free(reg);
			register_tmp_saved[i] = -1;
			/* load into regsiter */
			printf("\tmovl\t%d(%%ebp),%s\n",TMP_OFF(i),register_tmp_names[rr]);
			return rr;
		}
	error("reg is not found");
	return -1;
}

void register_save(int reg) {
	int i;

	if (register_tmp_state[reg] < 0)
		return;
	for (i = 0; i < N_SAVE; i++)
		if (register_tmp_saved[i] < 0) {
			printf("\tmovl\t%s,%d(%%ebp)\n",register_tmp_names[reg],TMP_OFF(reg));
			register_tmp_saved[i] = register_tmp_state[reg];
			register_tmp_state[reg] = -1;
			return;
		}
	error("no free temp save");
}

void register_save_all() {
	int i;
	for (i = 0; i < N_REG; i++)
		register_save(i);
}

void register_assign(int reg, int value) {
	if (register_tmp_state[reg] == value)
		return;
	register_save(reg);
	register_tmp_state[reg] = value;
}

void register_free(int reg) {
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

void generate_im_code_one_operand(int opcode, int operand_int_1) {
	ThreeAddressCode *code = intermediate_code_new();
	code->opcode = opcode;
	code->operand_int_1 = operand_int_1;
	intermediate_code_add(code);
}

void generate_im_code_two_operand(int opcode, int operand_int_1, int operand_int_2) {
	ThreeAddressCode *code = intermediate_code_new();
	code->opcode = opcode;
	code->operand_int_1 = operand_int_1;
	code->operand_int_2 = operand_int_2;
	intermediate_code_add(code);
}

void generate_im_code_three_operand(int opcode, int operand_int_1, int operand_int_2, int operand_int_3) {
	ThreeAddressCode *code = intermediate_code_new();
	code->opcode = opcode;
	code->operand_int_1 = operand_int_1;
	code->operand_int_2 = operand_int_2;
	code->operand_int_3 = operand_int_3;
	intermediate_code_add(code);
}

void generate_im_code_two_operand_and_string(int opcode, int operand_int_1, int operand_int_2, char *s) {
	ThreeAddressCode *code = intermediate_code_new();
	code->opcode = opcode;
	code->operand_int_1 = operand_int_1;
	code->operand_int_2 = operand_int_2;
	code->operand_str = s;
	intermediate_code_add(code);
}

int generate_string(char *s) {
	int l = label_counter++;
	printf("\t.section\t.rodata\n");
	printf(".LC%d:\n",l);
	printf("\t.string %s\n",s);
	return l;
}

void emit_function_code(char *entry_name, int n_local) {
	int operand_int_1, operand_int_2, operand_int_3;
	int reg, reg1, reg2;
	char *operand_str;
	int ret_lab, label1, label2;
	int frame_size;

	/* function header */
	puts("\t.text");
	puts("\t.align\t4");
	printf("\t.globl\t%s\n", entry_name);
	printf("\t.type\t%s,@function\n", entry_name);
	printf("%s:\n", entry_name);
	printf("\tpushl\t%%ebp\n");
	printf("\tmovl\t%%esp,%%ebp\n");
	frame_size = -LOCAL_VAR_OFF(n_local);
	ret_lab = label_counter++;
	printf("\tsubl\t$%d,%%esp\n",frame_size);
	printf("\tmovl\t%%ebx,-4(%%ebp)\n");

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
			printf("\tmovl\t$%d,%s\n",operand_int_2,register_tmp_names[reg]);
			break;
		case OP_LOADA:	/* load arg */
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			printf("\tmovl\t%d(%%ebp),%s\n",ARG_OFF(operand_int_2),register_tmp_names[reg]);
			break;
		case OP_LOADL:	/* load local */
			if(operand_int_1 < 0)
				break;
			reg = register_get_free(operand_int_1);
			printf("\tmovl\t%d(%%ebp),%s\n",LOCAL_VAR_OFF(operand_int_2),register_tmp_names[reg]);
			break;
		case OP_STOREA:	/* store arg */
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tmovl\t%s,%d(%%ebp)\n",register_tmp_names[reg],ARG_OFF(operand_int_2));
			break;
		case OP_STOREL:	/* store local */
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tmovl\t%s,%d(%%ebp)\n",register_tmp_names[reg],LOCAL_VAR_OFF(operand_int_2));
			break;
		case OP_BEQ0:	/* conditional branch */
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tcmpl\t$0,%s\n",register_tmp_names[reg]);
			printf("\tje\t.L%d\n",operand_int_2);
			break;
		case OP_LABEL:
			printf(".L%d:\n",operand_int_1);
			break;
		case OP_JUMP:
			printf("\tjmp\t.L%d\n",operand_int_1);
			break;
		case OP_CALL:
			register_save_all();
			printf("\tcall\t%s\n",operand_str);
			if(operand_int_1 < 0)
				break;
			register_assign(REG_AX, operand_int_1);
			printf("\tadd $%d,%%esp\n",operand_int_2 * 4);
			break;
		case OP_ARG:
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tpushl\t%s\n",register_tmp_names[reg]);
			break;
		case OP_RET:
			reg = register_use(operand_int_1);
			register_free(reg);
			if(reg != REG_AX)
				printf("\tmovl\t%s,%%eax\n",register_tmp_names[reg]);
			printf("\tjmp .L%d\n",ret_lab);
			break;
		case OP_ADD:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\taddl\t%s,%s\n",register_tmp_names[reg2],register_tmp_names[reg1]);
			break;
		case OP_SUB:
			reg1 = register_use(operand_int_2);
			reg2 = register_use(operand_int_3);
			register_free(reg1);
			register_free(reg2);
			if(operand_int_1 < 0)
				break;
			register_assign(reg1, operand_int_1);
			printf("\tsubl\t%s,%s\n",register_tmp_names[reg2],register_tmp_names[reg1]);
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
				printf("\tmovl %s,%s\n",register_tmp_names[reg1],register_tmp_names[REG_AX]);
			printf("\timull\t%s,%s\n",register_tmp_names[reg2],register_tmp_names[REG_AX]);
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
			printf("\tcmpl\t%s,%s\n",register_tmp_names[reg2],register_tmp_names[reg1]);
			printf("\tjl .L%d\n",label1);
			printf("\tmovl\t$0,%s\n",register_tmp_names[reg]);
			printf("\tjmp .L%d\n",label2);
			printf(".L%d:\tmovl\t$1,%s\n",label1,register_tmp_names[reg]);
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
			printf("\tcmpl\t%s,%s\n",register_tmp_names[reg2],register_tmp_names[reg1]);
			printf("\tjg .L%d\n",label1);
			printf("\tmovl\t$0,%s\n",register_tmp_names[reg]);
			printf("\tjmp .L%d\n",label2);
			printf(".L%d:\tmovl\t$1,%s\n",label1,register_tmp_names[reg]);
			printf(".L%d:",label2);
			break;

		case OP_PRINT:
			reg = register_use(operand_int_1);
			register_free(reg);
			printf("\tpushl\t%s\n",register_tmp_names[reg]);
			printf("\tlea\t.LC%d,%s\n",operand_int_2, register_tmp_names[reg]);
			printf("\tpushl\t%s\n",register_tmp_names[reg]);
			register_save_all();
			printf("\tcall\tprintln\n");
			printf("\taddl\t$8,%%esp\n");
			break;
		}
	}
	// Reset
	intermediate_code_head = NULL;
	intermediate_code_tail = NULL;
	/* return sequence */
	printf(".L%d:\tmovl\t-4(%%ebp), %%ebx\n",ret_lab);
	printf("\tleave\n");
	printf("\tret\n");
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

void compile_compound_statement(ParseTreeNode *local_vars, ParseTreeNode *statements) {
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

int compile_function_arguments(ParseTreeNode *args, int i) {
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

void compile_function_call(int target, Symbol *f, ParseTreeNode *args) {
	int narg;
	narg = compile_function_arguments(args,0);
	generate_im_code_two_operand_and_string(OP_CALL, target, narg, f->name);
}

void compile_load_store_variable(Symbol *sym, int r, int loadstore) {
	Variable *var = NULL;
	for (var = variables_current; var; var = var->prev) {
		if (var->sym == sym) {
			switch (var->type) {
			case VAR_ARGUMENT:
				if (loadstore == OP_LOAD)
					generate_im_code_two_operand(OP_LOADA, r, var->pos);
				else
					generate_im_code_two_operand(OP_STOREA, r, var->pos);
				return;
			case VAR_LOCAL:
				if (loadstore == OP_LOAD)
					generate_im_code_two_operand(OP_LOADL, r, var->pos);
				else
					generate_im_code_two_operand(OP_STOREL, r, var->pos);
				return;
			}
		}
	}
	error("undefined variable\n");
}

void emit_function_call(ParseTreeNode *args) {
	int l = generate_string(get_list_node_nth_left(args,0)->string);
	int r = register_counter++;
	compile_expression(r,get_list_node_nth_left(args,1));
	generate_im_code_two_operand(OP_PRINT,r,l);
}

void compile_expression(int target, ParseTreeNode *node) {
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
	case NODE_ADDITION:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_ADD,target,reg1,reg2);
		return;
	case NODE_SUBTRACTION:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_SUB,target,reg1,reg2);
		return;
	case NODE_MULTIPLICATION:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_MUL,target,reg1,reg2);
		return;
	case NODE_LESS_THAN:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_LT,target,reg1,reg2);
		return;
	case NODE_GREATER_THAN:
		reg1 = register_counter++;
		reg2 = register_counter++;
		compile_expression(reg1,node->left);
		compile_expression(reg2,node->right);
		generate_im_code_three_operand(OP_GT,target,reg1,reg2);
		return;
	case NODE_FUNCTION_CALL:
		compile_function_call(target, identifier_get(node->left), node->right);
		return;
	case NODE_PRINTLN_CALL:
		if(target != -1)
			error("println has no value");
		emit_function_call(node->left);
		return;
	default:
		error("unknown operater/statement");
	}
}

void compile_return_statement(ParseTreeNode *expr) {
	int r;
	if(expr != NULL) {
		r = register_counter++;
		compile_expression(r,expr);
	}
	else
		r = -1;
	generate_im_code_one_operand(OP_RET,r);
}

void compile_if_statement(ParseTreeNode *cond, ParseTreeNode *then_part, ParseTreeNode *else_part) {
	int label1,label2;
	int r = register_counter++;
	compile_expression(r,cond);
	label1 = label_counter++;
	generate_im_code_two_operand(OP_BEQ0,r,label1);
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

void compile_while_statement(ParseTreeNode *cond,ParseTreeNode *body) {
	int label1 = label_counter++;
	int label2 = label_counter++;
	int r = register_counter++;

	generate_im_code_one_operand(OP_LABEL,label1);
	compile_expression(r,cond);
	generate_im_code_two_operand(OP_BEQ0,r,label2);
	compile_statement(body);
	generate_im_code_one_operand(OP_JUMP,label1);
	generate_im_code_one_operand(OP_LABEL,label2);
}

void compile_statement(ParseTreeNode *node) {
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

void compile_function(Symbol *fsym, ParseTreeNode *params, ParseTreeNode *body) {
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

