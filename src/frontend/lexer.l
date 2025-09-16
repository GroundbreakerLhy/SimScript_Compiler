%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.h"

extern int yylineno;
extern int yycolumn;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option yylineno
%option noyywrap
%option case-insensitive

DIGIT       [0-9]
LETTER      [a-zA-Z]
IDENTIFIER  {LETTER}({LETTER}|{DIGIT}|_)*
INTEGER     {DIGIT}+
FLOAT       {DIGIT}+\.{DIGIT}+
STRING      \"([^\"\\]|\\.)*\"
COMMENT     ''.*
WHITESPACE  [ \t]+

%%

{COMMENT}           { /* ignore comments */ }
{WHITESPACE}        { /* ignore whitespace */ }
\n                  { yycolumn = 1; return NEWLINE; }

"PREAMBLE"          { return PREAMBLE; }
"END"               { return END; }
"MAIN"              { return MAIN; }
"DEFINE"            { return DEFINE; }
"ENTITY"            { return ENTITY; }
"EVENT"             { return EVENT; }
"ATTRIBUTES"        { return ATTRIBUTES; }
"PARAMETERS"        { return PARAMETERS; }
"AS"                { return AS; }
"INT"               { return INT; }
"REAL"              { return REAL; }
"DOUBLE"            { return DOUBLE; }
"TEXT"              { return TEXT; }
"ALPHA"             { return ALPHA; }
"SET"               { return SET; }
"VOID"              { return VOID; }
"IF"                { return IF; }
"THEN"              { return THEN; }
"ELSE"              { return ELSE; }
"ELSEIF"            { return ELSEIF; }
"ALWAYS"            { return ALWAYS; }
"FOR"               { return FOR; }
"TO"                { return TO; }
"STEP"              { return STEP; }
"DO"                { return DO; }
"LOOP"              { return LOOP; }
"WHILE"             { return WHILE; }
"EACH"              { return EACH; }
"IN"                { return IN; }
"INHERITS"          { return INHERITS; }
"NEW"               { return NEW; }
"OVERRIDE"          { return OVERRIDE; }
"ROUTINE"           { return ROUTINE; }
"RETURN"            { return RETURN; }
"LET"               { return LET; }
"AND"               { return AND; }
"OR"                { return OR; }
"NOT"               { return NOT; }
"WRITE"             { return WRITE; }
"READ"              { return READ_KW; }
"SCREEN"            { return SCREEN; }
"OPEN"              { return OPEN; }
"CLOSE"             { return CLOSE; }
"CLASS"             { return CLASS; }
"FILE"              { return FILE_KW; }
"START"             { return START; }
"SIMULATION"        { return SIMULATION; }
"SCHEDULE"          { return SCHEDULE; }
"SUPER"             { return SUPER; }
"THIS"              { return THIS; }
"TIME"              { return TIME; }
"ADVANCE"           { return ADVANCE; }
"BY"                { return BY; }
"AT"                { return AT; }
"WITH"              { return WITH; }
"ADD"               { return ADD; }
"REMOVE"            { return REMOVE; }
"FROM"              { return FROM; }
"PARALLEL"          { return PARALLEL; }
"SECTIONS"          { return SECTIONS; }
"CRITICAL"          { return CRITICAL; }
"BARRIER"           { return BARRIER; }
"MASTER"            { return MASTER; }
"SINGLE"            { return SINGLE; }
"THREADPRIVATE"     { return THREADPRIVATE; }
"RANDOM"            { return RANDOM; }
"UNIFORM"           { return UNIFORM; }
"NORMAL"            { return NORMAL; }
"EXPONENTIAL"       { return EXPONENTIAL; }
"POISSON"           { return POISSON; }

"="                 { return ASSIGN; }
"+"                 { return PLUS; }
"-"                 { return MINUS; }
"*"                 { return MULTIPLY; }
"/"                 { return DIVIDE; }
"**"                { return POWER; }
"<>"                { return NE; }
"<="                { return LE; }
">="                { return GE; }
"<"                 { return LT; }
">"                 { return GT; }

"("                 { return LPAREN; }
")"                 { return RPAREN; }
","                 { return COMMA; }
":"                 { return COLON; }
";"                 { return SEMICOLON; }
"."                 { return DOT; }

{INTEGER}           { yylval.ival = atoi(yytext); return INTEGER_LITERAL; }
{FLOAT}             { yylval.fval = atof(yytext); return FLOAT_LITERAL; }
{STRING}            { 
                      yylval.sval = strdup(yytext + 1);
                      yylval.sval[strlen(yylval.sval) - 1] = '\0';
                      return STRING_LITERAL; 
                    }
{IDENTIFIER}        { yylval.sval = strdup(yytext); return IDENTIFIER; }

.                   { fprintf(stderr, "Unknown character: %s at line %d\n", yytext, yylineno); }

%%
