
program :=  {external_definition}*

external_definition:=
   function_name '(' [ parameter {',' parameter}* ] ')' compound_statement
 | VAR variable_name ['=' expr] ';'
 | VAR array_name '[' expr ']' ';'

compound_statement:=
        '{' [local_variable_declaration] {statement}* '}'

local_variable_declaration: =
         VAR variable_name [ {',' variable_name}* ] ';'

statement :=
  expr ';'
 | compound_statement
 | IF '(' expr ')' statement [ ELSE statement ]
 | RETURN [expr ] ';'
 | WHILE '(' expr ')' statement
 | FOR '(' expr ';' expr ';' expr ')' statement

expr:=   primary_expr
 | variable_name '=' expr
 | array_name '[' expr ']' '=' expr
 | expr '+' expr
 | expr '-' expr
 | expr '*' expr
 | expr '<' expr
 | expr '>' expr

primary_expr:=
   variable_name
 | NUMBER
 | STRING
 | array_name '[' expr ']'
 | function_name '(' expr [{',' expr}*] ')'
 | function_name '(' ')'
 | '(' expr ')'
 | PRINTLN  '(' STRING ',' expr ')'

