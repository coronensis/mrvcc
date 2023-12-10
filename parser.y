%{
#include "compiler.c"
%}

%union {
    ParseTreeNode *node;
}

%token TOK_IDENTIFIER
%token TOK_NUMBER
%token TOK_STRING
%token TOK_INTEGER
%token TOK_IF
%token TOK_ELSE
%token TOK_RETURN
%token TOK_WHILE
%token TOK_LOGICAL_AND
%token TOK_LOGICAL_OR
%token TOK_LE
%token TOK_GE
%token TOK_EQ
%token TOK_NE
%token TOK_SLL
%token TOK_SRL
%token TOK_INC
%token TOK_DEC

%nonassoc LOWER_THAN_ELSE
%nonassoc TOK_ELSE

%type <node> TOK_IDENTIFIER TOK_NUMBER TOK_STRING
%type <node> primary_expression unary_expression multiplicative_expression shift_expression additive_expression equality_expression
%type <node> relational_expression bitwise_and_expression xor_expression bitwise_or_expression logical_and_expression
%type <node> logical_or_expression assignment_expression expression
%type <node> compound_statement statements_list statement
%type <node> variable_definitions identifiers_list parameters_list arguments_list

%start translation_unit

%%

primary_expression
	: TOK_IDENTIFIER
	| TOK_NUMBER
	| TOK_STRING
	| TOK_IDENTIFIER '(' arguments_list ')' 	    { $$ = new_op_node(NODE_CALL, $1, $3); }
	| TOK_IDENTIFIER '(' ')' 			    { $$ = new_op_node(NODE_CALL, $1, NULL); }
        | '(' expression ')' 				    { $$ = $2; }
	;

unary_expression
	: primary_expression
	| TOK_INC unary_expression 			    { $$ = new_op_node(NODE_INC, $2, NULL); }
	| TOK_DEC unary_expression 			    { $$ = new_op_node(NODE_DEC, $2, NULL); }
	| '+' unary_expression 				    { $$ = new_op_node(NODE_UPLUS, $2, NULL); }
	| '-' unary_expression 				    { $$ = new_op_node(NODE_UMINUS, $2, NULL); }
	| '!' unary_expression 				    { $$ = new_op_node(NODE_LOGICAL_NOT, $2, NULL); }
	| '~' unary_expression 				    { $$ = new_op_node(NODE_BITWISE_NOT, $2, NULL); }
	| '*' unary_expression 				    { $$ = new_op_node(NODE_DEREF, $2, NULL); }
	| '&' unary_expression 				    { $$ = new_op_node(NODE_ADDR, $2, NULL); }
	;

multiplicative_expression
	: unary_expression
	| multiplicative_expression '*' unary_expression    { $$ = new_op_node(NODE_MUL, $1, $3); }
	| multiplicative_expression '/' unary_expression    { $$ = new_op_node(NODE_DIV, $1, $3); }
	| multiplicative_expression '%' unary_expression    { $$ = new_op_node(NODE_MOD, $1, $3); }
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression { $$ = new_op_node(NODE_ADD, $1, $3); }
	| additive_expression '-' multiplicative_expression { $$ = new_op_node(NODE_SUB, $1, $3); }
	;

shift_expression
	: additive_expression
	| shift_expression TOK_SLL additive_expression      { $$ = new_op_node(NODE_SLL, $1, $3); }
	| shift_expression TOK_SRL additive_expression	    { $$ = new_op_node(NODE_SRL, $1, $3); }
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression 	    { $$ = new_op_node(NODE_LT, $1, $3); }
	| relational_expression '>' shift_expression	    { $$ = new_op_node(NODE_GT, $1, $3); }
	| relational_expression TOK_LE shift_expression	    { $$ = new_op_node(NODE_LE, $1, $3); }
	| relational_expression TOK_GE shift_expression	    { $$ = new_op_node(NODE_GE, $1, $3); }
	;

equality_expression
	: relational_expression
	| equality_expression TOK_EQ relational_expression  { $$ = new_op_node(NODE_EQ, $1, $3); }
	| equality_expression TOK_NE relational_expression  { $$ = new_op_node(NODE_NE, $1, $3); }
	;

bitwise_and_expression
	: equality_expression
	| bitwise_and_expression '&' equality_expression    { $$ = new_op_node(NODE_BITWISE_AND, $1, $3); }
	;

xor_expression
	: bitwise_and_expression
	| xor_expression '^' bitwise_and_expression	    { $$ = new_op_node(NODE_XOR, $1, $3); }
	;

bitwise_or_expression
	: xor_expression
	| bitwise_or_expression '|' xor_expression	    { $$ = new_op_node(NODE_BITWISE_OR, $1, $3); }
	;

logical_and_expression
	: bitwise_or_expression
	| logical_and_expression TOK_LOGICAL_AND bitwise_or_expression { $$ = new_op_node(NODE_LOGICAL_AND, $1, $3); }
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression TOK_LOGICAL_OR logical_and_expression  { $$ = new_op_node(NODE_LOGICAL_OR, $1, $3); }
	;

assignment_expression
	: logical_or_expression
	| TOK_IDENTIFIER '=' logical_or_expression		       { $$ = new_op_node(NODE_ASSIGNMENT, $1, $3); }
	;

expression
	: assignment_expression
	;

arguments_list
	: expression 				                       { $$ = new_op_node(NODE_LIST, $1, NULL); }
	| arguments_list ',' expression 	                       { $$ = new_list_node_add_last($1, $3); }
	;

statement
	: expression ';'
	| compound_statement
	| TOK_IF '(' expression ')' statement %prec LOWER_THAN_ELSE { $$ = new_op_node(NODE_IF, $3, new_op_node(NODE_LIST, $5, new_op_node(NODE_LIST, NULL, NULL))); }
        | TOK_IF '(' expression ')' statement TOK_ELSE statement    { $$ = new_op_node(NODE_IF, $3, new_op_node(NODE_LIST, $5, new_op_node(NODE_LIST, $7, NULL))); }
	| TOK_RETURN expression ';'			            { $$ = new_op_node(NODE_RETURN, $2, NULL); }
	| TOK_RETURN ';' 				            { $$ = new_op_node(NODE_RETURN, NULL, NULL); }
	| TOK_WHILE '(' expression ')' statement 		    { $$ = new_op_node(NODE_WHILE, $3, $5); }
	;

statements_list
	: statement					    { $$ = new_op_node(NODE_LIST, $1, NULL); }
	| statements_list statement			    { $$ = new_list_node_add_last($1, $2); }
	;

variable_definitions
	: /* empty */					    { $$ = NULL; }
	| TOK_INTEGER identifiers_list ';'		    { $$ = $2; }
	;

compound_statement
    : '{' variable_definitions statements_list '}'	    { $$ = new_op_node(NODE_COMPOUND, $2, $3); }
	;

identifiers_list
	: TOK_IDENTIFIER				    { $$ = new_op_node(NODE_LIST, $1, NULL); }
	| identifiers_list ',' TOK_IDENTIFIER		    { $$ = new_list_node_add_last($1, $3); }
	;

parameters_list
	: '(' ')'					    { $$ = NULL; }
	| '(' identifiers_list ')'			    { $$ = $2; }
	;

function_definition
	: TOK_IDENTIFIER parameters_list compound_statement { compile_function(identifier_get($1), $2, $3); }
	;

function_definitions
	: function_definition
	| function_definitions function_definition
	;

translation_unit
	: /* empty */
	| function_definitions
	;

%%

// -----------------------------------------------------------------------------
// lexer
// -----------------------------------------------------------------------------

#include "lex.yy.c"

// -----------------------------------------------------------------------------
// driver
// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
	if (argc > 1)
		stdin = fopen(argv[1], "r");
    yyparse();
    return 0;
}

