D   [0-9]
L   [a-zA-Z_]
H   [a-fA-F0-9]

%{
#include <stdio.h>
#include "y.tab.h"

void comment(void);
%}

%%

"/*"              { comment(); }

"int"		  { return TOK_INTEGER; }
"if"		  { return TOK_IF; }
"else"		  { return TOK_ELSE; }
"while"		  { return TOK_WHILE; }
"return"	  { return TOK_RETURN; }

{L}({L}|{D})*	  { yylval.node = new_identifier_node(yytext); return TOK_IDENTIFIER; }

{D}+		  { yylval.node = new_number_node(yytext); return TOK_NUMBER; }

\"(\\.|[^\\"])*\" { yylval.node = new_string_node(yytext); return TOK_STRING; }

"++"		  { return TOK_INC; }
"--"		  { return TOK_DEC; }
"<<"		  { return TOK_SLL; }
">>"		  { return TOK_SRL; }
"&&"		  { return TOK_LOGICAL_AND; }
"||"		  { return TOK_LOGICAL_OR; }
"<="		  { return TOK_LE; }
">="		  { return TOK_GE; }
"=="		  { return TOK_EQ; }
"!="		  { return TOK_NE; }

"+"		  { return '+'; }
"-"		  { return '-'; }
"*"		  { return '*'; }
"/"		  { return '/'; }
"%"		  { return '%'; }
"&"		  { return '&'; }
"|"		  { return '|'; }
"^"		  { return '^'; }
"~"		  { return '~'; }
">"		  { return '>'; }
"<"		  { return '<'; }
"("		  { return '('; }
")"		  { return ')'; }
"{"		  { return '{'; }
"}"		  { return '}'; }
"["		  { return '['; }
"]"		  { return ']'; }
";"		  { return ';'; }
","		  { return ','; }
"="		  { return '='; }

[ \t\v\n\f]	  { /* ignore whitespace */     }
.		  { /* ignore bad characters */ }

%%

int yywrap(void)
{
	return 1 ;
}

void comment(void)
{
        char c, c1;
        while ((c = input()))
                if ((c == '*') && ((c1 = input()))) {
                        if (c1 != '/')
                                unput(c1);
                        else
                                break;
                }

}

