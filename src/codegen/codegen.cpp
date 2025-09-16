#include "codegen.h"
#include "../frontend/symbol_table.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 代码生成器结构 */
struct CodeGenerator {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMValueRef current_function;
    SymbolTable* symbol_table;
};

/* 前向声明 */
static LLVMValueRef codegen_expression(CodeGenerator* codegen, ASTNode* node);
static void codegen_statement(CodeGenerator* codegen, ASTNode* node);
static void codegen_function(CodeGenerator* codegen, ASTNode* node);

/* 创建代码生成器 */
CodeGenerator* codegen_create(const char* module_name) {
    CodeGenerator* codegen = (CodeGenerator*)malloc(sizeof(CodeGenerator));
    if (!codegen) return NULL;
    
    codegen->context = LLVMContextCreate();
    codegen->module = LLVMModuleCreateWithNameInContext(module_name, codegen->context);
    codegen->builder = LLVMCreateBuilderInContext(codegen->context);
    codegen->current_function = NULL;
    codegen->symbol_table = symbol_table_create();
    
    return codegen;
}

/* 销毁代码生成器 */
void codegen_destroy(CodeGenerator* codegen) {
    if (!codegen) return;
    
    if (codegen->symbol_table) symbol_table_destroy(codegen->symbol_table);
    if (codegen->builder) LLVMDisposeBuilder(codegen->builder);
    if (codegen->module) LLVMDisposeModule(codegen->module);
    if (codegen->context) LLVMContextDispose(codegen->context);
    free(codegen);
}

/* 获取 LLVM 类型 */
static LLVMTypeRef get_llvm_type(CodeGenerator* codegen, DataType type) {
    switch (type) {
        case TYPE_INT:
            return LLVMInt32TypeInContext(codegen->context);
        case TYPE_REAL:
        case TYPE_DOUBLE:
            return LLVMDoubleTypeInContext(codegen->context);
        case TYPE_TEXT:
        case TYPE_ALPHA:
            return LLVMPointerType(LLVMInt8TypeInContext(codegen->context), 0);
        case TYPE_SET:
            // SET类型表示为指向结构的指针，结构包含元素数组和大小
            {
                LLVMTypeRef int_type = LLVMInt32TypeInContext(codegen->context);
                LLVMTypeRef ptr_type = LLVMPointerType(LLVMInt32TypeInContext(codegen->context), 0);
                LLVMTypeRef field_types[] = {ptr_type, int_type, int_type}; // elements, size, capacity
                LLVMTypeRef set_struct = LLVMStructTypeInContext(codegen->context, field_types, 3, 0);
                return LLVMPointerType(set_struct, 0);
            }
        default:
            return LLVMVoidTypeInContext(codegen->context);
    }
}

/* 从LLVM值推断SIMSCRIPT类型 */
static DataType infer_type_from_llvm_value(CodeGenerator* codegen, LLVMValueRef value) {
    if (!value) return TYPE_INT;
    
    LLVMTypeRef type = LLVMTypeOf(value);
    LLVMTypeKind kind = LLVMGetTypeKind(type);
    
    switch (kind) {
        case LLVMIntegerTypeKind:
            return TYPE_INT;
        case LLVMDoubleTypeKind:
            return TYPE_REAL;
        case LLVMPointerTypeKind:
            return TYPE_TEXT; // 假设指针类型是字符串
        default:
            return TYPE_INT;
    }
}

/* 生成表达式 */
static LLVMValueRef codegen_expression(CodeGenerator* codegen, ASTNode* node) {
    if (!node) return NULL;
    
    switch (node->type) {
        case NODE_INTEGER_LITERAL:
            return LLVMConstInt(LLVMInt32TypeInContext(codegen->context), 
                               node->data.integer_literal.value, 0);
            
        case NODE_FLOAT_LITERAL:
            return LLVMConstReal(LLVMDoubleTypeInContext(codegen->context), 
                                node->data.float_literal.value);
            
        case NODE_STRING_LITERAL:
            return LLVMBuildGlobalStringPtr(codegen->builder, 
                                          node->data.string_literal.value, "str");
            
        case NODE_IDENTIFIER: {
            // 查找变量
            Symbol* symbol = symbol_table_lookup(codegen->symbol_table, node->data.identifier.name);
            if (!symbol) {
                fprintf(stderr, "Error: Undefined variable '%s'\n", node->data.identifier.name);
                return NULL;
            }
            if (!symbol->is_initialized) {
                fprintf(stderr, "Error: Variable '%s' used before initialization\n", node->data.identifier.name);
                return NULL;
            }
            return LLVMBuildLoad2(codegen->builder, get_llvm_type(codegen, symbol->type), 
                                (LLVMValueRef)symbol->llvm_value, symbol->name);
        }
            
        case NODE_BINARY_EXPRESSION: {
            LLVMValueRef left = codegen_expression(codegen, node->data.binary_expression.left);
            LLVMValueRef right = codegen_expression(codegen, node->data.binary_expression.right);
            
            if (!left || !right) return NULL;
            
            switch (node->data.binary_expression.op) {
                case BINOP_ADD:
                    return LLVMBuildAdd(codegen->builder, left, right, "add");
                case BINOP_SUB:
                    return LLVMBuildSub(codegen->builder, left, right, "sub");
                case BINOP_MUL:
                    return LLVMBuildMul(codegen->builder, left, right, "mul");
                case BINOP_DIV:
                    return LLVMBuildSDiv(codegen->builder, left, right, "div");
                case BINOP_EQ:
                    return LLVMBuildICmp(codegen->builder, LLVMIntEQ, left, right, "eq");
                case BINOP_NE:
                    return LLVMBuildICmp(codegen->builder, LLVMIntNE, left, right, "ne");
                case BINOP_LT:
                    return LLVMBuildICmp(codegen->builder, LLVMIntSLT, left, right, "lt");
                case BINOP_GT:
                    return LLVMBuildICmp(codegen->builder, LLVMIntSGT, left, right, "gt");
                case BINOP_LE:
                    return LLVMBuildICmp(codegen->builder, LLVMIntSLE, left, right, "le");
                case BINOP_GE:
                    return LLVMBuildICmp(codegen->builder, LLVMIntSGE, left, right, "ge");
                case BINOP_AND:
                    return LLVMBuildAnd(codegen->builder, left, right, "and");
                case BINOP_OR:
                    return LLVMBuildOr(codegen->builder, left, right, "or");
                default:
                    return NULL;
            }
        }
        
        case NODE_UNARY_EXPRESSION: {
            LLVMValueRef operand = codegen_expression(codegen, node->data.unary_expression.operand);
            if (!operand) return NULL;
            
            switch (node->data.unary_expression.op) {
                case UNOP_NOT:
                    return LLVMBuildNot(codegen->builder, operand, "not");
                case UNOP_MINUS:
                    return LLVMBuildNeg(codegen->builder, operand, "neg");
                default:
                    return NULL;
            }
        }
        
        case NODE_FUNCTION_CALL: {
            // 查找函数
            LLVMValueRef func = LLVMGetNamedFunction(codegen->module, node->data.function_call.name);
            if (!func) {
                fprintf(stderr, "Error: Undefined function '%s'\n", node->data.function_call.name);
                return NULL;
            }
            
            // 准备参数
            LLVMValueRef* args = NULL;
            int arg_count = 0;
            
            if (node->data.function_call.arguments && node->data.function_call.arguments->type == NODE_EXPRESSION_LIST) {
                arg_count = node->data.function_call.arguments->data.list.count;
                args = (LLVMValueRef*)malloc(arg_count * sizeof(LLVMValueRef));
                
                for (int i = 0; i < arg_count; i++) {
                    args[i] = codegen_expression(codegen, node->data.function_call.arguments->data.list.items[i]);
                    if (!args[i]) {
                        free(args);
                        return NULL;
                    }
                }
            }
            
            LLVMTypeRef func_type = LLVMGetElementType(LLVMTypeOf(func));
            LLVMValueRef result = LLVMBuildCall2(codegen->builder, func_type, func, args, arg_count, "call");
            
            if (args) free(args);
            return result;
        }
        
        case NODE_SET_CREATION: {
            // 创建集合字面量
            // TODO: 实现集合创建的LLVM代码生成
            return NULL;
        }
        
        case NODE_SET_OPERATION: {
            // 处理集合操作
            // TODO: 实现集合操作的LLVM代码生成
            return NULL;
        }
        
        default:
            return NULL;
    }
}

/* 生成语句 */
static void codegen_statement(CodeGenerator* codegen, ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_VARIABLE_DECLARATION: {
            // 处理变量声明
            const char* name = node->data.variable_declaration.name;
            DataType type = node->data.variable_declaration.type;
            
            // 添加到符号表
            if (!symbol_table_add(codegen->symbol_table, name, type)) {
                fprintf(stderr, "Error: Variable '%s' already declared\n", name);
                return;
            }
            
            // 在当前函数中分配局部变量
            LLVMTypeRef llvm_type = get_llvm_type(codegen, type);
            LLVMValueRef alloca = LLVMBuildAlloca(codegen->builder, llvm_type, name);
            
            Symbol* symbol = symbol_table_lookup(codegen->symbol_table, name);
            symbol_table_set_value(symbol, alloca);
            
            // 如果有初始化表达式
            if (node->data.variable_declaration.initializer) {
                LLVMValueRef init_value = codegen_expression(codegen, node->data.variable_declaration.initializer);
                if (init_value) {
                    LLVMBuildStore(codegen->builder, init_value, alloca);
                    symbol->is_initialized = 1;
                }
            }
            break;
        }
        
        case NODE_ASSIGNMENT: {
            // 处理赋值语句 - 也需要处理变量声明
            const char* target = node->data.assignment.target;
            Symbol* symbol = symbol_table_lookup(codegen->symbol_table, target);
            
            // 如果变量不存在，推断类型并创建
            if (!symbol) {
                // 先计算右侧的值来推断类型
                LLVMValueRef value = codegen_expression(codegen, node->data.assignment.value);
                if (!value) return;
                
                // 从LLVM值推断类型
                DataType inferred_type = infer_type_from_llvm_value(codegen, value);
                
                // 添加到符号表
                if (!symbol_table_add(codegen->symbol_table, target, inferred_type)) {
                    fprintf(stderr, "Error: Failed to declare variable '%s'\n", target);
                    return;
                }
                
                symbol = symbol_table_lookup(codegen->symbol_table, target);
                LLVMTypeRef llvm_type = get_llvm_type(codegen, inferred_type);
                LLVMValueRef alloca = LLVMBuildAlloca(codegen->builder, llvm_type, target);
                symbol_table_set_value(symbol, alloca);
                
                // 存储值
                LLVMBuildStore(codegen->builder, value, alloca);
                symbol->is_initialized = 1;
            } else {
                // 变量已存在，执行赋值
                LLVMValueRef value = codegen_expression(codegen, node->data.assignment.value);
                if (value) {
                    LLVMBuildStore(codegen->builder, value, (LLVMValueRef)symbol->llvm_value);
                    symbol->is_initialized = 1;
                }
            }
            break;
        }
            
        case NODE_WRITE: {
            LLVMValueRef expr = codegen_expression(codegen, node->data.write_stmt.expression);
            if (expr) {
                // 查找或创建 printf 函数
                LLVMValueRef printf_func = LLVMGetNamedFunction(codegen->module, "printf");
                if (!printf_func) {
                    LLVMTypeRef param_types[] = {LLVMPointerType(LLVMInt8TypeInContext(codegen->context), 0)};
                    LLVMTypeRef printf_type = LLVMFunctionType(
                        LLVMInt32TypeInContext(codegen->context),
                        param_types, 1, 1);
                    printf_func = LLVMAddFunction(codegen->module, "printf", printf_type);
                }
                
                // 根据表达式类型选择格式字符串
                LLVMTypeRef expr_type = LLVMTypeOf(expr);
                LLVMTypeKind kind = LLVMGetTypeKind(expr_type);
                
                LLVMValueRef format;
                if (kind == LLVMDoubleTypeKind) {
                    format = LLVMBuildGlobalStringPtr(codegen->builder, "%.2f\n", "fmt");
                } else if (kind == LLVMPointerTypeKind) {
                    format = LLVMBuildGlobalStringPtr(codegen->builder, "%s\n", "fmt");
                } else {
                    format = LLVMBuildGlobalStringPtr(codegen->builder, "%d\n", "fmt");
                }
                
                LLVMValueRef args[] = {format, expr};
                
                LLVMTypeRef printf_type = LLVMGetElementType(LLVMTypeOf(printf_func));
                LLVMBuildCall2(codegen->builder, printf_type, printf_func, args, 2, "");
            }
            break;
        }
        
        case NODE_RETURN: {
            // 处理 return 语句
            if (node->data.return_stmt.value) {
                LLVMValueRef return_value = codegen_expression(codegen, node->data.return_stmt.value);
                if (return_value) {
                    LLVMBuildRet(codegen->builder, return_value);
                }
            } else {
                LLVMBuildRetVoid(codegen->builder);
            }
            break;
        }
        
        case NODE_ENTITY_DECLARATION: {
            // 处理实体声明
            const char* entity_name = node->data.entity_declaration.name;
            ASTNode* attributes = node->data.entity_declaration.attributes;
            
            // 添加实体到符号表
            if (!symbol_table_add_entity(codegen->symbol_table, entity_name, attributes)) {
                fprintf(stderr, "Error: Entity '%s' already declared\n", entity_name);
                return;
            }
            
            // 创建结构体类型
            int field_count = 0;
            LLVMTypeRef* field_types = NULL;
            
            if (attributes && attributes->type == NODE_ATTRIBUTE_LIST) {
                field_count = attributes->data.list.count;
                field_types = (LLVMTypeRef*)malloc(field_count * sizeof(LLVMTypeRef));
                
                for (int i = 0; i < field_count; i++) {
                    ASTNode* attr = attributes->data.list.items[i];
                    if (attr->type == NODE_ATTRIBUTE) {
                        field_types[i] = get_llvm_type(codegen, attr->data.attribute.type);
                    }
                }
            }
            
            LLVMTypeRef struct_type = LLVMStructTypeInContext(codegen->context, field_types, field_count, 0);
            // Note: LLVMStructSetName is not available in C API, we can use LLVMStructCreateNamed instead
            // For now, we'll just store the type reference
            (void)struct_type; // Suppress unused variable warning
            
            if (field_types) free(field_types);
            break;
        }
        
        case NODE_EVENT_DECLARATION: {
            // 处理事件声明
            const char* event_name = node->data.event_declaration.name;
            ASTNode* parameters = node->data.event_declaration.parameters;
            
            // 添加事件到符号表
            if (!symbol_table_add_event(codegen->symbol_table, event_name, parameters)) {
                fprintf(stderr, "Error: Event '%s' already declared\n", event_name);
                return;
            }
            
            // 事件在SIMSCRIPT中通常是用于调度的，这里我们可以创建一个函数类型来表示
            // 构建参数类型列表
            LLVMTypeRef* param_types = NULL;
            int param_count = 0;
            
            if (parameters && parameters->type == NODE_PARAMETER_LIST) {
                param_count = parameters->data.list.count;
                param_types = (LLVMTypeRef*)malloc(param_count * sizeof(LLVMTypeRef));
                
                for (int i = 0; i < param_count; i++) {
                    ASTNode* param = parameters->data.list.items[i];
                    if (param->type == NODE_PARAMETER) {
                        param_types[i] = get_llvm_type(codegen, param->data.parameter.type);
                    }
                }
            }
            
            // 事件处理函数返回void
            LLVMTypeRef event_func_type = LLVMFunctionType(LLVMVoidTypeInContext(codegen->context), param_types, param_count, 0);
            // Store function type for later use if needed
            (void)event_func_type; // Suppress unused variable warning
            
            if (param_types) free(param_types);
            break;
        }
            break;
            
        case NODE_IF: {
            // IF-THEN-ELSE 语句
            LLVMValueRef condition = codegen_expression(codegen, node->data.if_stmt.condition);
            if (!condition) break;
            
            // 创建基本块
            LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "then");
            LLVMBasicBlockRef else_block = NULL;
            LLVMBasicBlockRef merge_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "merge");
            
            if (node->data.if_stmt.else_branch) {
                else_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "else");
                LLVMBuildCondBr(codegen->builder, condition, then_block, else_block);
            } else {
                LLVMBuildCondBr(codegen->builder, condition, then_block, merge_block);
            }
            
            // 生成 then 分支
            LLVMPositionBuilderAtEnd(codegen->builder, then_block);
            codegen_statement(codegen, node->data.if_stmt.then_branch);
            // 只有在没有终结指令时才添加分支
            if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(codegen->builder))) {
                LLVMBuildBr(codegen->builder, merge_block);
            }
            
            // 生成 else 分支（如果存在）
            if (else_block) {
                LLVMPositionBuilderAtEnd(codegen->builder, else_block);
                codegen_statement(codegen, node->data.if_stmt.else_branch);
                // 只有在没有终结指令时才添加分支
                if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(codegen->builder))) {
                    LLVMBuildBr(codegen->builder, merge_block);
                }
            }
            
            // 继续在 merge 块
            LLVMPositionBuilderAtEnd(codegen->builder, merge_block);
            break;
        }
        
        case NODE_WHILE: {
            // WHILE 循环
            LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "while_cond");
            LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "while_body");
            LLVMBasicBlockRef exit_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "while_exit");
            
            // 跳转到条件块
            LLVMBuildBr(codegen->builder, cond_block);
            
            // 生成条件块
            LLVMPositionBuilderAtEnd(codegen->builder, cond_block);
            LLVMValueRef condition = codegen_expression(codegen, node->data.while_stmt.condition);
            if (condition) {
                LLVMBuildCondBr(codegen->builder, condition, body_block, exit_block);
            }
            
            // 生成循环体
            LLVMPositionBuilderAtEnd(codegen->builder, body_block);
            codegen_statement(codegen, node->data.while_stmt.body);
            LLVMBuildBr(codegen->builder, cond_block);
            
            // 继续在退出块
            LLVMPositionBuilderAtEnd(codegen->builder, exit_block);
            break;
        }
        
        case NODE_FOR: {
            // FOR 循环
            const char* var_name = node->data.for_stmt.variable;
            
            // 创建循环变量
            Symbol* loop_var = symbol_table_lookup(codegen->symbol_table, var_name);
            if (!loop_var) {
                symbol_table_add(codegen->symbol_table, var_name, TYPE_INT);
                loop_var = symbol_table_lookup(codegen->symbol_table, var_name);
                LLVMValueRef alloca = LLVMBuildAlloca(codegen->builder, LLVMInt32TypeInContext(codegen->context), var_name);
                symbol_table_set_value(loop_var, alloca);
            }
            
            // 初始化循环变量
            LLVMValueRef start_val = codegen_expression(codegen, node->data.for_stmt.start);
            if (start_val) {
                LLVMBuildStore(codegen->builder, start_val, (LLVMValueRef)loop_var->llvm_value);
                loop_var->is_initialized = 1;
            }
            
            // 创建基本块
            LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "for_cond");
            LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "for_body");
            LLVMBasicBlockRef incr_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "for_incr");
            LLVMBasicBlockRef exit_block = LLVMAppendBasicBlockInContext(codegen->context, codegen->current_function, "for_exit");
            
            LLVMBuildBr(codegen->builder, cond_block);
            
            // 条件检查
            LLVMPositionBuilderAtEnd(codegen->builder, cond_block);
            LLVMValueRef current_val = LLVMBuildLoad2(codegen->builder, LLVMInt32TypeInContext(codegen->context), 
                                                    (LLVMValueRef)loop_var->llvm_value, "loop_var");
            LLVMValueRef end_val = codegen_expression(codegen, node->data.for_stmt.end);
            if (end_val) {
                LLVMValueRef cmp = LLVMBuildICmp(codegen->builder, LLVMIntSLE, current_val, end_val, "for_cmp");
                LLVMBuildCondBr(codegen->builder, cmp, body_block, exit_block);
            }
            
            // 循环体
            LLVMPositionBuilderAtEnd(codegen->builder, body_block);
            codegen_statement(codegen, node->data.for_stmt.body);
            LLVMBuildBr(codegen->builder, incr_block);
            
            // 增量
            LLVMPositionBuilderAtEnd(codegen->builder, incr_block);
            LLVMValueRef step_val = LLVMConstInt(LLVMInt32TypeInContext(codegen->context), 1, 0);
            if (node->data.for_stmt.step) {
                step_val = codegen_expression(codegen, node->data.for_stmt.step);
            }
            current_val = LLVMBuildLoad2(codegen->builder, LLVMInt32TypeInContext(codegen->context), 
                                       (LLVMValueRef)loop_var->llvm_value, "loop_var");
            LLVMValueRef next_val = LLVMBuildAdd(codegen->builder, current_val, step_val, "next_val");
            LLVMBuildStore(codegen->builder, next_val, (LLVMValueRef)loop_var->llvm_value);
            LLVMBuildBr(codegen->builder, cond_block);
            
            // 退出
            LLVMPositionBuilderAtEnd(codegen->builder, exit_block);
            break;
        }
        
        case NODE_FUNCTION_DECLARATION: {
            // 在语句中处理函数声明时跳过（应在preamble中处理）
            break;
        }
        
        case NODE_STATEMENT_LIST:
            for (int i = 0; i < node->data.list.count; i++) {
                codegen_statement(codegen, node->data.list.items[i]);
            }
            break;
            
        case NODE_FOR_EACH: {
            // FOR EACH 循环 - 简化实现，假设集合是一个数组
            const char* var_name = node->data.for_each_stmt.variable;
            
            // 创建循环变量
            Symbol* loop_var = symbol_table_lookup(codegen->symbol_table, var_name);
            if (!loop_var) {
                symbol_table_add(codegen->symbol_table, var_name, TYPE_INT);
                loop_var = symbol_table_lookup(codegen->symbol_table, var_name);
                LLVMValueRef alloca = LLVMBuildAlloca(codegen->builder, LLVMInt32TypeInContext(codegen->context), var_name);
                symbol_table_set_value(loop_var, alloca);
            }
            
            // TODO: 实现真正的集合迭代逻辑
            // 这里只是一个占位符实现
            fprintf(stderr, "Warning: FOR EACH not fully implemented yet\n");
            codegen_statement(codegen, node->data.for_each_stmt.body);
            break;
        }
        
        case NODE_WRITE_TO_FILE: {
            // 写入文件 - 简化实现
            fprintf(stderr, "Warning: WRITE TO FILE not fully implemented yet\n");
            break;
        }
        
        case NODE_OPEN_FILE: {
            // 打开文件 - 简化实现
            fprintf(stderr, "Warning: OPEN FILE not fully implemented yet\n");
            break;
        }
        
        case NODE_CLOSE_FILE: {
            // 关闭文件 - 简化实现
            fprintf(stderr, "Warning: CLOSE FILE not fully implemented yet\n");
            break;
        }
        
        case NODE_READ_FROM_FILE: {
            // 从文件读取 - 简化实现
            fprintf(stderr, "Warning: READ FROM FILE not fully implemented yet\n");
            break;
        }
        
        case NODE_START_SIMULATION: {
            // 开始仿真 - 简化实现
            fprintf(stderr, "Info: START SIMULATION encountered\n");
            break;
        }
        
        case NODE_SCHEDULE: {
            // 调度事件 - 简化实现
            fprintf(stderr, "Info: SCHEDULE event '%s'\n", node->data.schedule_stmt.event_name);
            break;
        }
        
        case NODE_ADVANCE_TIME: {
            // 推进时间 - 简化实现
            fprintf(stderr, "Info: ADVANCE TIME\n");
            break;
        }
        
        case NODE_CLASS_DECLARATION: {
            // 处理类声明
            const char* class_name = node->data.class_declaration.name;
            const char* parent_class = node->data.class_declaration.parent_class;
            ASTNode* members = node->data.class_declaration.members;
            
            // 添加类到符号表
            if (!symbol_table_add_class(codegen->symbol_table, class_name, parent_class)) {
                fprintf(stderr, "Error: Class '%s' already declared\n", class_name);
                return;
            }
            
            // 处理类成员（变量和方法）
            if (members && members->type == NODE_STATEMENT_LIST) {
                for (int i = 0; i < members->data.statement_list.count; i++) {
                    ASTNode* member = members->data.statement_list.items[i];
                    if (member->type == NODE_VARIABLE_DECLARATION) {
                        // 添加成员变量
                        Symbol* class_symbol = symbol_table_lookup(codegen->symbol_table, class_name);
                        SymbolTable* member_table = (SymbolTable*)class_symbol->members;
                        symbol_table_add_member(member_table, 
                                               member->data.variable_declaration.name,
                                               member->data.variable_declaration.type);
                    } else if (member->type == NODE_METHOD_DECLARATION) {
                        // 添加方法
                        Symbol* class_symbol = symbol_table_lookup(codegen->symbol_table, class_name);
                        SymbolTable* method_table = (SymbolTable*)class_symbol->methods;
                        symbol_table_add_method(method_table,
                                               member->data.method_declaration.name,
                                               member->data.method_declaration.return_type,
                                               member->data.method_declaration.parameters,
                                               member->data.method_declaration.is_override);
                    }
                }
            }
            break;
        }
        
        case NODE_OBJECT_CREATION: {
            // 处理对象创建
            const char* var_name = node->data.object_creation.variable_name;
            const char* class_name = node->data.object_creation.class_name;
            ASTNode* arguments = node->data.object_creation.arguments;
            
            // 查找类
            Symbol* class_symbol = symbol_table_lookup(codegen->symbol_table, class_name);
            if (!class_symbol) {
                fprintf(stderr, "Error: Class '%s' not found\n", class_name);
                return;
            }
            
            // 创建对象（简化实现：使用结构体指针）
            // 这里需要更复杂的实现来处理构造函数调用
            // 暂时只创建变量声明
            if (!symbol_table_add(codegen->symbol_table, var_name, TYPE_VOID)) {
                fprintf(stderr, "Error: Variable '%s' already declared\n", var_name);
                return;
            }
            
            Symbol* var_symbol = symbol_table_lookup(codegen->symbol_table, var_name);
            // 分配内存（简化）
            LLVMTypeRef void_ptr_type = LLVMPointerType(LLVMInt8TypeInContext(codegen->context), 0);
            LLVMValueRef alloca = LLVMBuildAlloca(codegen->builder, void_ptr_type, var_name);
            symbol_table_set_value(var_symbol, alloca);
            
            break;
        }
        
        case NODE_METHOD_CALL: {
            // 处理方法调用
            const char* object_name = node->data.method_call.object_name;
            const char* method_name = node->data.method_call.method_name;
            ASTNode* arguments = node->data.method_call.arguments;
            
            // 查找对象
            Symbol* object_symbol = symbol_table_lookup(codegen->symbol_table, object_name);
            if (!object_symbol) {
                fprintf(stderr, "Error: Object '%s' not found\n", object_name);
                return;
            }
            
            // 这里需要实现方法查找和调用
            // 暂时跳过，等待更完整的实现
            fprintf(stderr, "Info: Method call %s.%s\n", object_name, method_name);
            break;
        }
            
        default:
            break;
    }
}

/* 生成函数 */
static void codegen_function(CodeGenerator* codegen, ASTNode* node) {
    if (!node || node->type != NODE_FUNCTION_DECLARATION) return;
    
    const char* func_name = node->data.function_declaration.name;
    DataType return_type = node->data.function_declaration.return_type;
    ASTNode* parameters = node->data.function_declaration.parameters;
    ASTNode* body = node->data.function_declaration.body;
    
    // 构建参数类型列表
    LLVMTypeRef* param_types = NULL;
    int param_count = 0;
    
    if (parameters && parameters->type == NODE_PARAMETER_LIST) {
        param_count = parameters->data.list.count;
        param_types = (LLVMTypeRef*)malloc(param_count * sizeof(LLVMTypeRef));
        
        for (int i = 0; i < param_count; i++) {
            ASTNode* param = parameters->data.list.items[i];
            if (param->type == NODE_PARAMETER) {
                param_types[i] = get_llvm_type(codegen, param->data.parameter.type);
            }
        }
    }
    
    // 创建函数类型
    LLVMTypeRef llvm_return_type = get_llvm_type(codegen, return_type);
    LLVMTypeRef func_type = LLVMFunctionType(llvm_return_type, param_types, param_count, 0);
    
    // 创建函数
    LLVMValueRef function = LLVMAddFunction(codegen->module, func_name, func_type);
    
    // 创建入口基本块
    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlockInContext(codegen->context, function, "entry");
    
    // 保存当前状态
    LLVMValueRef old_function = codegen->current_function;
    LLVMBasicBlockRef old_insert_block = LLVMGetInsertBlock(codegen->builder);
    
    // 设置新的函数上下文
    codegen->current_function = function;
    LLVMPositionBuilderAtEnd(codegen->builder, entry_block);
    
    // 为参数创建 alloca 并存储参数值
    if (parameters && parameters->type == NODE_PARAMETER_LIST) {
        for (int i = 0; i < param_count; i++) {
            ASTNode* param = parameters->data.list.items[i];
            if (param->type == NODE_PARAMETER) {
                const char* param_name = param->data.parameter.name;
                DataType param_type = param->data.parameter.type;
                
                // 添加参数到符号表
                symbol_table_add(codegen->symbol_table, param_name, param_type);
                Symbol* symbol = symbol_table_lookup(codegen->symbol_table, param_name);
                
                // 创建 alloca
                LLVMTypeRef param_llvm_type = get_llvm_type(codegen, param_type);
                LLVMValueRef alloca = LLVMBuildAlloca(codegen->builder, param_llvm_type, param_name);
                symbol_table_set_value(symbol, alloca);
                
                // 存储参数值
                LLVMValueRef param_val = LLVMGetParam(function, i);
                LLVMBuildStore(codegen->builder, param_val, alloca);
                symbol->is_initialized = 1;
            }
        }
    }
    
    // 生成函数体
    if (body) {
        codegen_statement(codegen, body);
    }
    
    // 如果没有显式的 return 语句，添加默认返回
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(codegen->builder);
    if (current_block && !LLVMGetBasicBlockTerminator(current_block)) {
        if (return_type == TYPE_VOID) {
            LLVMBuildRetVoid(codegen->builder);
        } else {
            // 为非void函数添加默认返回值
            LLVMValueRef default_val;
            if (return_type == TYPE_INT) {
                default_val = LLVMConstInt(LLVMInt32TypeInContext(codegen->context), 0, 0);
            } else if (return_type == TYPE_REAL || return_type == TYPE_DOUBLE) {
                default_val = LLVMConstReal(LLVMDoubleTypeInContext(codegen->context), 0.0);
            } else {
                default_val = LLVMConstNull(llvm_return_type);
            }
            LLVMBuildRet(codegen->builder, default_val);
        }
    }
    
    // 恢复之前的状态
    codegen->current_function = old_function;
    if (old_insert_block) {
        LLVMPositionBuilderAtEnd(codegen->builder, old_insert_block);
    }
    
    // 清理
    if (param_types) free(param_types);
}

/* 生成代码 */
int codegen_generate(CodeGenerator* codegen, ASTNode* ast) {
    if (!codegen || !ast) return 0;
    
    // 创建 main 函数
    LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32TypeInContext(codegen->context), NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(codegen->module, "main", main_type);
    LLVMBasicBlockRef main_block = LLVMAppendBasicBlockInContext(codegen->context, main_func, "entry");
    LLVMPositionBuilderAtEnd(codegen->builder, main_block);
    
    codegen->current_function = main_func;
    
    // 生成 AST
    if (ast->type == NODE_PROGRAM) {
        if (ast->data.program.preamble) {
            // 处理 preamble 中的声明
            if (ast->data.program.preamble->type == NODE_STATEMENT_LIST) {
                for (int i = 0; i < ast->data.program.preamble->data.list.count; i++) {
                    ASTNode* decl = ast->data.program.preamble->data.list.items[i];
                    if (decl->type == NODE_FUNCTION_DECLARATION) {
                        codegen_function(codegen, decl);
                    } else if (decl->type == NODE_ENTITY_DECLARATION || 
                              decl->type == NODE_EVENT_DECLARATION ||
                              decl->type == NODE_CLASS_DECLARATION) {
                        // 处理实体、事件和类声明
                        codegen_statement(codegen, decl);
                    }
                }
            }
        }
        
        if (ast->data.program.main) {
            codegen_statement(codegen, ast->data.program.main);
        }
    } else {
        codegen_statement(codegen, ast);
    }
    
    // 返回 0
    LLVMBuildRet(codegen->builder, LLVMConstInt(LLVMInt32TypeInContext(codegen->context), 0, 0));
    
    // 验证模块
    char* error = NULL;
    if (LLVMVerifyModule(codegen->module, LLVMAbortProcessAction, &error)) {
        fprintf(stderr, "模块验证失败: %s\n", error);
        LLVMDisposeMessage(error);
        return 0;
    }
    
    return 1;
}

/* 输出 LLVM IR 到文件 */
int codegen_write_to_file(CodeGenerator* codegen, const char* filename) {
    if (!codegen) return 0;
    
    char* error = NULL;
    if (LLVMPrintModuleToFile(codegen->module, filename, &error)) {
        fprintf(stderr, "写入文件失败: %s\n", error);
        LLVMDisposeMessage(error);
        return 0;
    }
    
    return 1;
}

/* 输出 LLVM IR 到标准输出 */
void codegen_print_ir(CodeGenerator* codegen) {
    if (!codegen) return;
    
    char* ir = LLVMPrintModuleToString(codegen->module);
    printf("%s", ir);
    LLVMDisposeMessage(ir);
}
