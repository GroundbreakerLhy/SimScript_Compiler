#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Create symbol table */
SymbolTable* symbol_table_create() {
    SymbolTable* table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!table) return NULL;
    
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
    return table;
}

/* Destroy symbol table */
void symbol_table_destroy(SymbolTable* table) {
    if (!table) return;
    
    for (int i = 0; i < table->count; i++) {
        free(table->symbols[i].name);
    }
    free(table->symbols);
    free(table);
}

/* Expand symbol table */
static void expand_table(SymbolTable* table) {
    if (table->capacity == 0) {
        table->capacity = 8;
        table->symbols = (Symbol*)malloc(sizeof(Symbol) * table->capacity);
    } else {
        table->capacity *= 2;
        table->symbols = (Symbol*)realloc(table->symbols, sizeof(Symbol) * table->capacity);
    }
    
    if (!table->symbols) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
}

/* Add symbol to table */
int symbol_table_add(SymbolTable* table, const char* name, DataType type) {
    if (!table || !name) return 0;
    
    // Check if symbol already exists
    if (symbol_table_lookup(table, name)) {
        return 0; // Symbol already exists
    }
    
    if (table->count >= table->capacity) {
        expand_table(table);
    }
    
    Symbol* symbol = &table->symbols[table->count];
    symbol->name = strdup(name);
    symbol->type = type;
    symbol->is_initialized = 0;
    symbol->llvm_value = NULL;
    symbol->attributes = NULL;
    symbol->parameters = NULL;
    
    table->count++;
    return 1;
}

/* Add entity to table */
int symbol_table_add_entity(SymbolTable* table, const char* name, void* attributes) {
    if (!table || !name) return 0;
    
    // Check if symbol already exists
    if (symbol_table_lookup(table, name)) {
        return 0; // Symbol already exists
    }
    
    if (table->count >= table->capacity) {
        expand_table(table);
    }
    
    Symbol* symbol = &table->symbols[table->count];
    symbol->name = strdup(name);
    symbol->type = TYPE_VOID; // Entities don't have a specific type
    symbol->is_initialized = 1; // Entities are considered "initialized" upon declaration
    symbol->llvm_value = NULL;
    symbol->attributes = attributes;
    symbol->parameters = NULL;
    
    table->count++;
    return 1;
}

/* Add event to table */
int symbol_table_add_event(SymbolTable* table, const char* name, void* parameters) {
    if (!table || !name) return 0;
    
    // Check if symbol already exists
    if (symbol_table_lookup(table, name)) {
        return 0; // Symbol already exists
    }
    
    if (table->count >= table->capacity) {
        expand_table(table);
    }
    
    Symbol* symbol = &table->symbols[table->count];
    symbol->name = strdup(name);
    symbol->type = TYPE_VOID; // Events don't have a specific type
    symbol->is_initialized = 1; // Events are considered "initialized" upon declaration
    symbol->llvm_value = NULL;
    symbol->attributes = NULL;
    symbol->parameters = parameters;
    
    table->count++;
    return 1;
}

/* Add function to table */
int symbol_table_add_function(SymbolTable* table, const char* name, DataType return_type, void* parameters) {
    if (!table || !name) return 0;
    
    // Check if symbol already exists
    if (symbol_table_lookup(table, name)) {
        return 0; // Symbol already exists
    }
    
    if (table->count >= table->capacity) {
        expand_table(table);
    }
    
    Symbol* symbol = &table->symbols[table->count];
    symbol->name = strdup(name);
    symbol->type = return_type;
    symbol->is_initialized = 1; // Functions are considered "initialized" upon declaration
    symbol->llvm_value = NULL;
    symbol->attributes = NULL;
    symbol->parameters = parameters;
    
    table->count++;
    return 1;
}

/* Look up symbol in table */
Symbol* symbol_table_lookup(SymbolTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }
    return NULL;
}

/* Set symbol value */
void symbol_table_set_value(Symbol* symbol, void* llvm_value) {
    if (!symbol) return;
    symbol->llvm_value = llvm_value;
    symbol->is_initialized = 1;
}
