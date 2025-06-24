%{
/* SIMSCRIPT II.5 语法分析器 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylex();
extern int yylineno;
extern char* yytext;

void yyerror(const char* s);

ASTNode* root = NULL;
%}

%locations
%define parse.error verbose

%union {
    int ival;
    double fval;
    char* sval;
    ASTNode* node;
    DataType dtype;
}

/* 终结符 */
%token <ival> INTEGER_LITERAL
%token <fval> FLOAT_LITERAL
%token <sval> STRING_LITERAL IDENTIFIER
%token <dtype> INT REAL DOUBLE TEXT ALPHA SET

%token PREAMBLE END MAIN DEFINE ENTITY EVENT ATTRIBUTES PARAMETERS AS
%token IF THEN ELSE ELSEIF ALWAYS FOR TO STEP DO LOOP WHILE EACH IN
%token ROUTINE RETURN LET AND OR NOT WRITE READ_KW SCREEN ADD REMOVE FROM
%token OPEN CLOSE FILE_KW START SIMULATION SCHEDULE TIME ADVANCE BY AT WITH
%token ASSIGN PLUS MINUS MULTIPLY DIVIDE POWER
%token EQ NE LT GT LE GE
%token LPAREN RPAREN COMMA COLON SEMICOLON DOT NEWLINE

/* 非终结符 */
%type <node> program preamble main_section
%type <node> declaration_list declaration variable_declaration entity_declaration event_declaration function_declaration
%type <node> statement_list statement assignment_statement if_statement elseif_list while_statement for_statement return_statement write_statement
%type <node> file_statement open_statement close_statement read_statement
%type <node> simulation_statement start_simulation_statement schedule_statement advance_time_statement
%type <node> expression primary_expression binary_expression unary_expression function_call expression_list
%type <node> set_expression
%type <node> attribute_list attribute parameter_list parameter
%type <dtype> type

/* 运算符优先级 */
%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left MULTIPLY DIVIDE
%right POWER
%right NOT UMINUS

%start program

%%

program:
    preamble main_section { 
        root = create_program_node($1, $2); 
        $$ = root; 
    }
    | main_section { 
        root = create_program_node(NULL, $1); 
        $$ = root; 
    }
    ;

preamble:
    PREAMBLE newlines declaration_list END PREAMBLE newlines {
        $$ = $3;
    }
    ;

main_section:
    MAIN newlines statement_list END MAIN newlines {
        $$ = $3;
    }
    ;

declaration_list:
    declaration { 
        $$ = create_statement_list_node();
        add_statement_to_list($$, $1);
    }
    | declaration_list declaration {
        add_statement_to_list($1, $2);
        $$ = $1;
    }
    ;

declaration:
    variable_declaration newlines { $$ = $1; }
    | entity_declaration newlines { $$ = $1; }
    | event_declaration newlines { $$ = $1; }
    | function_declaration newlines { $$ = $1; }
    ;

variable_declaration:
    DEFINE IDENTIFIER AS type {
        $$ = create_variable_declaration_node($2, $4, NULL);
    }
    | DEFINE IDENTIFIER AS type ASSIGN expression {
        $$ = create_variable_declaration_node($2, $4, $6);
    }
    ;

entity_declaration:
    ENTITY IDENTIFIER newlines ATTRIBUTES newlines attribute_list newlines END {
        $$ = create_entity_declaration_node($2, $6);
    }
    ;

event_declaration:
    EVENT IDENTIFIER newlines PARAMETERS newlines parameter_list newlines END {
        $$ = create_event_declaration_node($2, $6);
    }
    ;

function_declaration:
    ROUTINE IDENTIFIER LPAREN parameter_list RPAREN ASSIGN type newlines statement_list END {
        $$ = create_function_declaration_node($2, $4, $7, $9);
    }
    | ROUTINE IDENTIFIER LPAREN RPAREN ASSIGN type newlines statement_list END {
        $$ = create_function_declaration_node($2, NULL, $6, $8);
    }
    ;

attribute_list:
    attribute {
        $$ = create_attribute_list_node();
        add_attribute_to_list($$, $1);
    }
    | attribute_list COMMA attribute {
        add_attribute_to_list($1, $3);
        $$ = $1;
    }
    | attribute_list COMMA newlines attribute {
        add_attribute_to_list($1, $4);
        $$ = $1;
    }
    ;

attribute:
    IDENTIFIER COLON type {
        $$ = create_attribute_node($1, $3);
    }
    ;

parameter_list:
    parameter {
        $$ = create_parameter_list_node();
        add_parameter_to_list($$, $1);
    }
    | parameter_list COMMA parameter {
        add_parameter_to_list($1, $3);
        $$ = $1;
    }
    | parameter_list COMMA newlines parameter {
        add_parameter_to_list($1, $4);
        $$ = $1;
    }
    | /* empty */ { $$ = NULL; }
    ;

parameter:
    IDENTIFIER COLON type {
        $$ = create_parameter_node($1, $3);
    }
    ;

statement_list:
    statement {
        $$ = create_statement_list_node();
        add_statement_to_list($$, $1);
    }
    | statement_list statement {
        add_statement_to_list($1, $2);
        $$ = $1;
    }
    | statement_list newlines {
        $$ = $1;
    }
    ;

statement:
    assignment_statement newlines { $$ = $1; }
    | if_statement newlines { $$ = $1; }
    | while_statement newlines { $$ = $1; }
    | for_statement newlines { $$ = $1; }
    | return_statement newlines { $$ = $1; }
    | write_statement newlines { $$ = $1; }
    | file_statement newlines { $$ = $1; }
    | simulation_statement newlines { $$ = $1; }
    ;

assignment_statement:
    LET IDENTIFIER ASSIGN expression {
        $$ = create_assignment_node($2, $4);
    }
    ;

if_statement:
    IF expression THEN newlines statement_list ALWAYS {
        $$ = create_if_node($2, $5, NULL);
    }
    | IF expression THEN newlines statement_list ELSE newlines statement_list ALWAYS {
        $$ = create_if_node($2, $5, $8);
    }
    | IF expression THEN newlines statement_list elseif_list ALWAYS {
        $$ = create_if_node($2, $5, $6);
    }
    | IF expression THEN newlines statement_list elseif_list ELSE newlines statement_list ALWAYS {
        ASTNode* else_node = create_if_node(NULL, $9, NULL);
        ASTNode* combined_else = combine_elseif_chain($6, else_node);
        $$ = create_if_node($2, $5, combined_else);
    }
    ;

elseif_list:
    ELSEIF expression THEN newlines statement_list {
        $$ = create_if_node($2, $5, NULL);
    }
    | elseif_list ELSEIF expression THEN newlines statement_list {
        ASTNode* new_elseif = create_if_node($3, $6, NULL);
        $$ = combine_elseif_chain($1, new_elseif);
    }
    ;

while_statement:
    WHILE expression DO newlines statement_list LOOP {
        $$ = create_while_node($2, $5);
    }
    ;

for_statement:
    FOR IDENTIFIER ASSIGN expression TO expression DO newlines statement_list LOOP {
        $$ = create_for_node($2, $4, $6, NULL, $9);
    }
    | FOR IDENTIFIER ASSIGN expression TO expression STEP expression DO newlines statement_list LOOP {
        $$ = create_for_node($2, $4, $6, $8, $11);
    }
    | FOR EACH IDENTIFIER IN expression DO newlines statement_list LOOP {
        $$ = create_for_each_node($3, $5, $8);
    }
    ;

return_statement:
    RETURN expression {
        $$ = create_return_node($2);
    }
    | RETURN {
        $$ = create_return_node(NULL);
    }
    ;

write_statement:
    WRITE expression TO SCREEN {
        $$ = create_write_node($2);
    }
    | WRITE expression TO FILE_KW STRING_LITERAL {
        $$ = create_write_to_file_node($2, $5);
    }
    ;

file_statement:
    open_statement { $$ = $1; }
    | close_statement { $$ = $1; }
    | read_statement { $$ = $1; }
    ;

open_statement:
    OPEN STRING_LITERAL AS FILE_KW INTEGER_LITERAL {
        $$ = create_open_file_node($2, $5);
    }
    ;

close_statement:
    CLOSE FILE_KW INTEGER_LITERAL {
        $$ = create_close_file_node($3);
    }
    ;

read_statement:
    READ_KW IDENTIFIER FROM FILE_KW INTEGER_LITERAL {
        $$ = create_read_from_file_node($2, $5);
    }
    ;

simulation_statement:
    start_simulation_statement { $$ = $1; }
    | schedule_statement { $$ = $1; }
    | advance_time_statement { $$ = $1; }
    ;

start_simulation_statement:
    START SIMULATION {
        $$ = create_start_simulation_node();
    }
    ;

schedule_statement:
    SCHEDULE IDENTIFIER AT expression {
        $$ = create_schedule_node($2, $4, NULL);
    }
    | SCHEDULE IDENTIFIER AT expression WITH expression {
        $$ = create_schedule_node($2, $4, $6);
    }
    ;

advance_time_statement:
    ADVANCE TIME BY expression {
        $$ = create_advance_time_node($4);
    }
    ;

expression:
    binary_expression { $$ = $1; }
    | unary_expression { $$ = $1; }
    | set_expression { $$ = $1; }
    | primary_expression { $$ = $1; }
    ;

binary_expression:
    expression PLUS expression {
        $$ = create_binary_expression_node($1, BINOP_ADD, $3);
    }
    | expression MINUS expression {
        $$ = create_binary_expression_node($1, BINOP_SUB, $3);
    }
    | expression MULTIPLY expression {
        $$ = create_binary_expression_node($1, BINOP_MUL, $3);
    }
    | expression DIVIDE expression {
        $$ = create_binary_expression_node($1, BINOP_DIV, $3);
    }
    | expression POWER expression {
        $$ = create_binary_expression_node($1, BINOP_POW, $3);
    }
    | expression EQ expression {
        $$ = create_binary_expression_node($1, BINOP_EQ, $3);
    }
    | expression NE expression {
        $$ = create_binary_expression_node($1, BINOP_NE, $3);
    }
    | expression LT expression {
        $$ = create_binary_expression_node($1, BINOP_LT, $3);
    }
    | expression GT expression {
        $$ = create_binary_expression_node($1, BINOP_GT, $3);
    }
    | expression LE expression {
        $$ = create_binary_expression_node($1, BINOP_LE, $3);
    }
    | expression GE expression {
        $$ = create_binary_expression_node($1, BINOP_GE, $3);
    }
    | expression AND expression {
        $$ = create_binary_expression_node($1, BINOP_AND, $3);
    }
    | expression OR expression {
        $$ = create_binary_expression_node($1, BINOP_OR, $3);
    }
    ;

unary_expression:
    NOT expression {
        $$ = create_unary_expression_node(UNOP_NOT, $2);
    }
    | MINUS expression %prec UMINUS {
        $$ = create_unary_expression_node(UNOP_MINUS, $2);
    }
    ;

primary_expression:
    INTEGER_LITERAL {
        $$ = create_integer_literal_node($1);
    }
    | FLOAT_LITERAL {
        $$ = create_float_literal_node($1);
    }
    | STRING_LITERAL {
        $$ = create_string_literal_node($1);
    }
    | IDENTIFIER {
        $$ = create_identifier_node($1);
    }
    | function_call {
        $$ = $1;
    }
    | LPAREN expression RPAREN {
        $$ = $2;
    }
    ;

function_call:
    IDENTIFIER LPAREN expression_list RPAREN {
        $$ = create_function_call_node($1, $3);
    }
    | IDENTIFIER LPAREN RPAREN {
        $$ = create_function_call_node($1, NULL);
    }
    ;

expression_list:
    expression {
        $$ = create_expression_list_node();
        add_expression_to_list($$, $1);
    }
    | expression_list COMMA expression {
        add_expression_to_list($1, $3);
        $$ = $1;
    }
    ;

set_expression:
    LPAREN expression_list RPAREN {
        /* 集合字面量 */
        $$ = create_set_creation_node($2);
    }
    | expression IN expression {
        /* 元素属于集合 */
        $$ = create_set_operation_node(SET_OP_CONTAINS, $1, $3);
    }
    | ADD expression IN expression {
        /* 添加元素到集合 */
        $$ = create_set_operation_node(SET_OP_ADD_ELEMENT, $2, $4);
    }
    | REMOVE expression FROM expression {
        /* 从集合中移除元素 */
        $$ = create_set_operation_node(SET_OP_REMOVE_ELEMENT, $2, $4);
    }
    ;

type:
    INT { $$ = TYPE_INT; }
    | REAL { $$ = TYPE_REAL; }
    | DOUBLE { $$ = TYPE_DOUBLE; }
    | TEXT { $$ = TYPE_TEXT; }
    | ALPHA { $$ = TYPE_ALPHA; }
    | SET { $$ = TYPE_SET; }
    ;

newlines:
    NEWLINE
    | newlines NEWLINE
    | /* empty */
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "语法错误在行 %d: %s\n", yylineno, s);
}
