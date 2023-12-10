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
%token TOK_PRINTLN

%nonassoc LOWER_THAN_ELSE
%nonassoc TOK_ELSE

%type <node> TOK_IDENTIFIER TOK_NUMBER TOK_STRING
%type <node> primary_expression multiplicative_expression additive_expression relational_expression assignment_expression expression
%type <node> compound_statement statements_list statement
%type <node> variable_definitions identifiers_list parameters_list arguments_list

%start translation_unit

%%

primary_expression
	: TOK_IDENTIFIER
	| TOK_NUMBER
	| TOK_STRING
	| TOK_IDENTIFIER '(' arguments_list ')'            { $$ = new_op_node(NODE_FUNCTION_CALL, $1, $3); }
	| TOK_IDENTIFIER '(' ')' 	                   { $$ = new_op_node(NODE_FUNCTION_CALL, $1, NULL); }
        | '(' expression ')' 		                   { $$ = $2; }
	| TOK_PRINTLN  '(' arguments_list ')'              { $$ = new_op_node(NODE_PRINTLN_CALL, $3, NULL); }
	;

multiplicative_expression
	: primary_expression
	| multiplicative_expression '*' primary_expression { $$ = new_op_node(NODE_MULTIPLICATION, $1, $3); }
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression { $$ = new_op_node(NODE_ADDITION, $1, $3); }
	| additive_expression '-' multiplicative_expression { $$ = new_op_node(NODE_SUBTRACTION, $1, $3); }
	;

relational_expression
	: additive_expression
	| relational_expression '<' additive_expression     { $$ = new_op_node(NODE_LESS_THAN, $1, $3); }
	| relational_expression '>' additive_expression     { $$ = new_op_node(NODE_GREATER_THAN, $1, $3); }
	;

assignment_expression
	: relational_expression
	| TOK_IDENTIFIER '=' relational_expression          { $$ = new_op_node(NODE_ASSIGNMENT, $1, $3); }
	;

expression
	: assignment_expression
	;

arguments_list
	: expression 				            { $$ = new_op_node(NODE_LIST, $1, NULL); }
	| arguments_list ',' expression 	            { $$ = new_list_node_add_last($1, $3); }
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

int main(int argc, char **argv)
{
        if (argc > 1) {
                stdin = fopen(argv[1], "r");
        }
        yyparse();
        return 0;
}

