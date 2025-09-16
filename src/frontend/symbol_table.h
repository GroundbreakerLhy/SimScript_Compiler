#pragma once

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Symbol table entry */
typedef struct Symbol {
    char* name;
    DataType type;
    int is_initialized;
    void* llvm_value;  /* LLVMValueRef */
    void* attributes;  /* For entities - ASTNode* pointing to attribute list */
    void* parameters;  /* For events/functions - ASTNode* pointing to parameter list */
    char* parent_class; /* For classes - name of parent class */
    void* methods;     /* For classes - SymbolTable* of methods */
    void* members;     /* For classes - SymbolTable* of member variables */
} Symbol;

/* Symbol table */
typedef struct SymbolTable {
    Symbol* symbols;
    int count;
    int capacity;
} SymbolTable;

/* Create symbol table */
SymbolTable* symbol_table_create();

/* Destroy symbol table */
void symbol_table_destroy(SymbolTable* table);

/* Add symbol to table */
int symbol_table_add(SymbolTable* table, const char* name, DataType type);

/* Add entity to table */
int symbol_table_add_entity(SymbolTable* table, const char* name, void* attributes);

/* Add event to table */
int symbol_table_add_event(SymbolTable* table, const char* name, void* parameters);

/* Add function to table */
int symbol_table_add_function(SymbolTable* table, const char* name, DataType return_type, void* parameters);

/* Add class to table */
int symbol_table_add_class(SymbolTable* table, const char* name, const char* parent_class);

/* Add method to class */
int symbol_table_add_method(SymbolTable* class_table, const char* name, DataType return_type, void* parameters, int is_override);

/* Add member to class */
int symbol_table_add_member(SymbolTable* class_table, const char* name, DataType type);

/* Look up symbol in table */
Symbol* symbol_table_lookup(SymbolTable* table, const char* name);

/* Set symbol value */
void symbol_table_set_value(Symbol* symbol, void* llvm_value);

#ifdef __cplusplus
}
#endif
