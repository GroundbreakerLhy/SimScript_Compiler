#include "codegen.h"
#include "../frontend/symbol_table.h"
#include "../debug/debug.h"
#include "../debug/debug_runtime.h"
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/ExecutionEngine.h>
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
    DebugContext* debug_ctx;
    // JIT 执行相关
    LLVMExecutionEngineRef execution_engine;
    int jit_initialized;
};

/* 辅助函数：记录变量值 */
static void debug_log_variable_value(CodeGenerator* codegen, const char* var_name, LLVMValueRef value) {
    if (!codegen->debug_ctx || !value) return;

    // 获取变量类型字符串
    const char* type_str = "unknown";
    LLVMTypeRef type = LLVMTypeOf(value);
    LLVMTypeKind kind = LLVMGetTypeKind(type);

    switch (kind) {
        case LLVMDoubleTypeKind: type_str = "REAL"; break;
        case LLVMIntegerTypeKind: type_str = "INT"; break;
        case LLVMPointerTypeKind: type_str = "TEXT"; break;
        default: type_str = "unknown"; break;
    }

    // 对于常量，我们可以直接获取值
    char value_str[64] = "unknown";
    if (LLVMIsConstant(value)) {
        if (kind == LLVMDoubleTypeKind) {
            double dval = LLVMConstRealGetDouble(value, NULL);
            snprintf(value_str, sizeof(value_str), "%.6f", dval);
        } else if (kind == LLVMIntegerTypeKind) {
            long long ival = LLVMConstIntGetSExtValue(value);
            snprintf(value_str, sizeof(value_str), "%lld", ival);
        } else if (kind == LLVMPointerTypeKind && LLVMIsGlobalConstant(value)) {
            // 对于字符串常量，显示为 string
            strcpy(value_str, "string");
        }
    } else {
        // 对于非常量值，显示为 runtime_value
        strcpy(value_str, "runtime_value");
    }

    debug_log_variable(codegen->debug_ctx, var_name, type_str, value_str);
}

/* 调试钩子函数：检查断点 */
static void debug_breakpoint_hook(CodeGenerator* codegen, int line_number, const char* function_name) {
    if (!codegen->debug_ctx) return;

    // 设置当前位置
    debug_set_location(codegen->debug_ctx, line_number, function_name);

    // 检查调试器状态
    DebuggerState state = debug_get_debugger_state(codegen->debug_ctx);
    if (state == DEBUGGER_BREAK || state == DEBUGGER_STEP) {
        // 进入调试器交互模式
        printf("\n=== DEBUG BREAKPOINT ===\n");
        printf("Location: %s:%d\n", function_name ? function_name : "<unknown>", line_number);
        printf("Type 'help' for commands, 'continue' to resume, 'quit' to exit\n");

        char command[256];
        while (1) {
            printf("(debug) ");
            if (fgets(command, sizeof(command), stdin) == NULL) break;

            // 移除换行符
            command[strcspn(command, "\n")] = 0;

            if (strcmp(command, "help") == 0) {
                printf("Available commands:\n");
                printf("  continue (c)    - Continue execution\n");
                printf("  step (s)        - Step to next instruction\n");
                printf("  breakpoints (b) - List breakpoints\n");
                printf("  variables (v)   - List local variables\n");
                printf("  print <var>     - Print variable value\n");
                printf("  memory <addr> <size> - Examine memory\n");
                printf("  quit (q)        - Exit debugger\n");
            } else if (debug_process_command(codegen->debug_ctx, command)) {
                if (debug_get_debugger_state(codegen->debug_ctx) == DEBUGGER_FINISHED) {
                    break;
                }
                if (strcmp(command, "continue") == 0 || strcmp(command, "c") == 0 ||
                    strcmp(command, "step") == 0 || strcmp(command, "s") == 0) {
                    break;
                }
            }
        }
    }
}

/* 分析循环是否适合OpenMP并行化 */
static int is_loop_suitable_for_parallelization(ASTNode* loop_body) {
    if (!loop_body) return 0;
    
    // 递归分析循环体
    switch (loop_body->type) {
        case NODE_STATEMENT_LIST: {
            // 检查语句列表中的每个语句
            for (int i = 0; i < loop_body->data.list.count; i++) {
                ASTNode* stmt = loop_body->data.list.items[i];
                
                // 如果包含文件I/O、屏幕输出或事件调度，不适合并行化
                if (stmt->type == NODE_WRITE || stmt->type == NODE_WRITE_TO_FILE ||
                    stmt->type == NODE_OPEN_FILE || stmt->type == NODE_CLOSE_FILE ||
                    stmt->type == NODE_READ_FROM_FILE || stmt->type == NODE_START_SIMULATION ||
                    stmt->type == NODE_SCHEDULE || stmt->type == NODE_ADVANCE_TIME) {
                    return 0; // 不适合并行化
                }
                
                // 如果包含嵌套循环，递归检查
                if (stmt->type == NODE_FOR || stmt->type == NODE_WHILE || stmt->type == NODE_FOR_EACH) {
                    if (!is_loop_suitable_for_parallelization(stmt->data.for_stmt.body)) {
                        return 0;
                    }
                }
                
                // 如果包含函数调用，需要进一步分析
                if (stmt->type == NODE_FUNCTION_CALL) {
                    // 假设大多数函数调用都是计算密集型的，除非是I/O相关的
                    // 这里可以添加更复杂的分析
                }
            }
            return 1; // 适合并行化
        }
        
        case NODE_ASSIGNMENT:
        case NODE_BINARY_EXPRESSION:
        case NODE_UNARY_EXPRESSION:
            // 纯计算操作，适合并行化
            return 1;
            
        case NODE_IF: {
            // 条件语句：检查then和else分支
            int then_suitable = is_loop_suitable_for_parallelization(loop_body->data.if_stmt.then_branch);
            int else_suitable = loop_body->data.if_stmt.else_branch ? 
                is_loop_suitable_for_parallelization(loop_body->data.if_stmt.else_branch) : 1;
            return then_suitable && else_suitable;
        }
        
        default:
            // 其他类型的语句默认认为不适合并行化，除非经过验证
            return 0;
    }
}

/* 创建代码生成器 */
CodeGenerator* codegen_create(const char* module_name) {
    return codegen_create_with_debug(module_name, NULL);
}

/* 创建带调试功能的代码生成器 */
CodeGenerator* codegen_create_with_debug(const char* module_name, DebugContext* debug_ctx) {
    CodeGenerator* codegen = (CodeGenerator*)malloc(sizeof(CodeGenerator));
    if (!codegen) return NULL;
    
    codegen->context = LLVMContextCreate();
    codegen->module = LLVMModuleCreateWithNameInContext(module_name, codegen->context);
    codegen->builder = LLVMCreateBuilderInContext(codegen->context);
    codegen->current_function = NULL;
    codegen->symbol_table = symbol_table_create();
    codegen->debug_ctx = debug_ctx;
    codegen->execution_engine = NULL;
    codegen->jit_initialized = 0;
    
    // 如果有调试上下文，声明调试运行时函数
    if (debug_ctx) {
        // 声明调试钩子函数
        LLVMTypeRef hook_params[] = {
            LLVMInt32TypeInContext(codegen->context),  // line_number
            LLVMPointerType(LLVMInt8TypeInContext(codegen->context), 0)  // function_name
        };
        LLVMTypeRef hook_type = LLVMFunctionType(LLVMVoidTypeInContext(codegen->context), hook_params, 2, 0);
        LLVMAddFunction(codegen->module, "simscript_debug_hook", hook_type);
    }
    
    return codegen;
}

/* 销毁代码生成器 */
void codegen_destroy(CodeGenerator* codegen) {
    if (!codegen) return;
    
    // 注意：不在这里清理JIT引擎，让它自然清理以避免段错误
    
    if (codegen->symbol_table) symbol_table_destroy(codegen->symbol_table);
    if (codegen->builder) LLVMDisposeBuilder(codegen->builder);
    if (codegen->module) LLVMDisposeModule(codegen->module);
    if (codegen->context) LLVMContextDispose(codegen->context);
    // 注意：不在这里销毁debug_ctx，它可能被其他地方使用
    free(codegen);
}

/* 获取调试上下文 */
DebugContext* codegen_get_debug_context(CodeGenerator* codegen) {
    return codegen ? codegen->debug_ctx : NULL;
}

/* 设置调试上下文 */
void codegen_set_debug_context(CodeGenerator* codegen, DebugContext* debug_ctx) {
    if (codegen) {
        codegen->debug_ctx = debug_ctx;
    }
}

/* ===== JIT 执行和调试功能实现 ===== */

/* 初始化JIT执行引擎 */
int codegen_init_jit(CodeGenerator* codegen) {
    if (!codegen || codegen->jit_initialized) return 1;
    
    char* error = NULL;
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    
    if (LLVMCreateExecutionEngineForModule(&codegen->execution_engine, codegen->module, &error) != 0) {
        fprintf(stderr, "Error: Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        return 0;
    }
    
    // 注册调试运行时函数
    if (codegen->debug_ctx) {
        LLVMAddGlobalMapping(codegen->execution_engine, 
                           LLVMGetNamedFunction(codegen->module, "simscript_debug_hook"), 
                           (void*)simscript_debug_hook);
        
        // 设置全局调试上下文
        simscript_debug_set_context(codegen->debug_ctx);
    }
    
    codegen->jit_initialized = 1;
    return 1;
}

/* 执行生成的代码（使用JIT） */
int codegen_execute_jit(CodeGenerator* codegen) {
    if (!codegen || !codegen->jit_initialized) {
        fprintf(stderr, "Error: JIT not initialized\n");
        return 0;
    }
    
    // 查找main函数
    LLVMValueRef main_func = LLVMGetNamedFunction(codegen->module, "main");
    if (!main_func) {
        fprintf(stderr, "Error: No main function found\n");
        return 0;
    }
    
    // 执行main函数
    typedef int (*MainFunc)();
    MainFunc func_ptr = (MainFunc)LLVMGetFunctionAddress(codegen->execution_engine, "main");
    if (!func_ptr) {
        fprintf(stderr, "Error: Failed to get function address\n");
        return 0;
    }
    
    int result = func_ptr();
    
    return 1;
}

/* 销毁JIT执行引擎 */
void codegen_destroy_jit(CodeGenerator* codegen) {
    if (codegen) {
        // 标记JIT已清理，实际清理在codegen_destroy中进行
        codegen->jit_initialized = 0;
    }
}

/* 检查是否支持JIT调试 */
int codegen_supports_debug_execution(CodeGenerator* codegen) {
    return codegen && codegen->debug_ctx != NULL;
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
            
            // 检查操作数类型
            LLVMTypeRef left_type = LLVMTypeOf(left);
            LLVMTypeRef right_type = LLVMTypeOf(right);
            
            // 确保类型匹配
            if (left_type != right_type) {
                fprintf(stderr, "Error: Type mismatch in binary expression\n");
                return NULL;
            }
            
            // 根据类型选择合适的运算指令
            int is_float = (left_type == LLVMFloatTypeInContext(codegen->context)) ||
                          (left_type == LLVMDoubleTypeInContext(codegen->context));
            
            switch (node->data.binary_expression.op) {
                case BINOP_ADD:
                    if (is_float) {
                        return LLVMBuildFAdd(codegen->builder, left, right, "fadd");
                    } else {
                        return LLVMBuildAdd(codegen->builder, left, right, "add");
                    }
                case BINOP_SUB:
                    if (is_float) {
                        return LLVMBuildFSub(codegen->builder, left, right, "fsub");
                    } else {
                        return LLVMBuildSub(codegen->builder, left, right, "sub");
                    }
                case BINOP_MUL:
                    if (is_float) {
                        return LLVMBuildFMul(codegen->builder, left, right, "fmul");
                    } else {
                        return LLVMBuildMul(codegen->builder, left, right, "mul");
                    }
                case BINOP_DIV:
                    if (is_float) {
                        return LLVMBuildFDiv(codegen->builder, left, right, "fdiv");
                    } else {
                        return LLVMBuildSDiv(codegen->builder, left, right, "div");
                    }
                case BINOP_EQ:
                    if (is_float) {
                        return LLVMBuildFCmp(codegen->builder, LLVMRealOEQ, left, right, "feq");
                    } else {
                        return LLVMBuildICmp(codegen->builder, LLVMIntEQ, left, right, "eq");
                    }
                case BINOP_NE:
                    if (is_float) {
                        return LLVMBuildFCmp(codegen->builder, LLVMRealONE, left, right, "fne");
                    } else {
                        return LLVMBuildICmp(codegen->builder, LLVMIntNE, left, right, "ne");
                    }
                case BINOP_LT:
                    if (is_float) {
                        return LLVMBuildFCmp(codegen->builder, LLVMRealOLT, left, right, "flt");
                    } else {
                        return LLVMBuildICmp(codegen->builder, LLVMIntSLT, left, right, "lt");
                    }
                case BINOP_GT:
                    if (is_float) {
                        return LLVMBuildFCmp(codegen->builder, LLVMRealOGT, left, right, "fgt");
                    } else {
                        return LLVMBuildICmp(codegen->builder, LLVMIntSGT, left, right, "gt");
                    }
                case BINOP_LE:
                    if (is_float) {
                        return LLVMBuildFCmp(codegen->builder, LLVMRealOLE, left, right, "fle");
                    } else {
                        return LLVMBuildICmp(codegen->builder, LLVMIntSLE, left, right, "le");
                    }
                case BINOP_GE:
                    if (is_float) {
                        return LLVMBuildFCmp(codegen->builder, LLVMRealOGE, left, right, "fge");
                    } else {
                        return LLVMBuildICmp(codegen->builder, LLVMIntSGE, left, right, "ge");
                    }
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
            
            // 调试：记录函数调用开始
            if (codegen->debug_ctx) {
                debug_log_function_call(codegen->debug_ctx, node->data.function_call.name, 0);
                debug_perf_start(codegen->debug_ctx, node->data.function_call.name);
                
                // 可视化：添加函数调用节点
                int func_node = debug_viz_add_node(codegen->debug_ctx, node->data.function_call.name, "ellipse", "lightgreen");
                if (func_node >= 0) {
                    // 记录节点ID以便后续连接
                }
                
                // 插入调试钩子调用
                LLVMValueRef hook_func = LLVMGetNamedFunction(codegen->module, "simscript_debug_hook");
                if (hook_func) {
                    LLVMValueRef line_num = LLVMConstInt(LLVMInt32TypeInContext(codegen->context), 0, 0);
                    LLVMValueRef func_name = LLVMBuildGlobalStringPtr(codegen->builder, node->data.function_call.name, "func_name");
                    LLVMValueRef hook_args[] = {line_num, func_name};
                    LLVMBuildCall2(codegen->builder, LLVMGetElementType(LLVMTypeOf(hook_func)), hook_func, hook_args, 2, "");
                }
            }            // 准备参数
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
            
    // 调试：记录函数调用结束
    if (codegen->debug_ctx) {
        debug_log_function_return(codegen->debug_ctx, node->data.function_call.name, NULL);
        debug_perf_end(codegen->debug_ctx, node->data.function_call.name);
    }            if (args) free(args);
            return result;
        }

        case NODE_STDLIB_FUNCTION_CALL: {
            // 处理标准库函数调用
            const char* func_name = node->data.stdlib_function_call.name;

    // 调试：记录标准库函数调用开始
    if (codegen->debug_ctx) {
        debug_log_function_call(codegen->debug_ctx, func_name, 0);
        debug_perf_start(codegen->debug_ctx, func_name);
    }            // 准备参数
            LLVMValueRef* args = NULL;
            int arg_count = 0;

            if (node->data.stdlib_function_call.arguments && node->data.stdlib_function_call.arguments->type == NODE_EXPRESSION_LIST) {
                arg_count = node->data.stdlib_function_call.arguments->data.list.count;
                args = (LLVMValueRef*)malloc(arg_count * sizeof(LLVMValueRef));

                for (int i = 0; i < arg_count; i++) {
                    args[i] = codegen_expression(codegen, node->data.stdlib_function_call.arguments->data.list.items[i]);
                    if (!args[i]) {
                        free(args);
                        return NULL;
                    }
                }
            }

            // 根据函数名生成相应的标准库调用
            LLVMValueRef result = NULL;

            if (strcmp(func_name, "random") == 0) {
                // random_uniform_global()
                LLVMValueRef func = LLVMGetNamedFunction(codegen->module, "random_uniform_global");
                if (!func) {
                    // 声明外部函数
                    LLVMTypeRef ret_type = LLVMDoubleTypeInContext(codegen->context);
                    LLVMTypeRef func_type = LLVMFunctionType(ret_type, NULL, 0, 0);
                    func = LLVMAddFunction(codegen->module, "random_uniform_global", func_type);
                }
                result = LLVMBuildCall2(codegen->builder, LLVMGetElementType(LLVMTypeOf(func)), func, NULL, 0, "random");
            }
            else if (strcmp(func_name, "uniform") == 0 && arg_count == 2) {
                // random_uniform_int_global(min, max)
                LLVMValueRef func = LLVMGetNamedFunction(codegen->module, "random_uniform_int_global");
                if (!func) {
                    LLVMTypeRef param_types[2] = {LLVMInt32TypeInContext(codegen->context), LLVMInt32TypeInContext(codegen->context)};
                    LLVMTypeRef ret_type = LLVMInt32TypeInContext(codegen->context);
                    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, 2, 0);
                    func = LLVMAddFunction(codegen->module, "random_uniform_int_global", func_type);
                }
                result = LLVMBuildCall2(codegen->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, arg_count, "uniform");
            }
            else if (strcmp(func_name, "normal") == 0 && arg_count == 2) {
                // random_normal_global(mean, stddev)
                LLVMValueRef func = LLVMGetNamedFunction(codegen->module, "random_normal_global");
                if (!func) {
                    LLVMTypeRef param_types[2] = {LLVMDoubleTypeInContext(codegen->context), LLVMDoubleTypeInContext(codegen->context)};
                    LLVMTypeRef ret_type = LLVMDoubleTypeInContext(codegen->context);
                    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, 2, 0);
                    func = LLVMAddFunction(codegen->module, "random_normal_global", func_type);
                }
                result = LLVMBuildCall2(codegen->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, arg_count, "normal");
            }
            else if (strcmp(func_name, "exponential") == 0 && arg_count == 1) {
                // random_exponential_global(rate)
                LLVMValueRef func = LLVMGetNamedFunction(codegen->module, "random_exponential_global");
                if (!func) {
                    LLVMTypeRef param_types[1] = {LLVMDoubleTypeInContext(codegen->context)};
                    LLVMTypeRef ret_type = LLVMDoubleTypeInContext(codegen->context);
                    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, 1, 0);
                    func = LLVMAddFunction(codegen->module, "random_exponential_global", func_type);
                }
                result = LLVMBuildCall2(codegen->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, arg_count, "exponential");
            }
            else if (strcmp(func_name, "poisson") == 0 && arg_count == 1) {
                // random_poisson_global(lambda)
                LLVMValueRef func = LLVMGetNamedFunction(codegen->module, "random_poisson_global");
                if (!func) {
                    LLVMTypeRef param_types[1] = {LLVMDoubleTypeInContext(codegen->context)};
                    LLVMTypeRef ret_type = LLVMInt32TypeInContext(codegen->context);
                    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, 1, 0);
                    func = LLVMAddFunction(codegen->module, "random_poisson_global", func_type);
                }
                result = LLVMBuildCall2(codegen->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, arg_count, "poisson");
            }
            else if (strcmp(func_name, "mean") == 0 && arg_count == 1) {
                // stats_mean(data, n) - 需要特殊处理数组参数
                fprintf(stderr, "Warning: stats_mean function not fully implemented in codegen\n");
                result = LLVMConstReal(LLVMDoubleTypeInContext(codegen->context), 0.0);
            }
            else if (strcmp(func_name, "seed") == 0 && arg_count == 1) {
                // random_seed(seed)
                LLVMValueRef func = LLVMGetNamedFunction(codegen->module, "random_seed");
                if (!func) {
                    LLVMTypeRef param_types[1] = {LLVMInt64TypeInContext(codegen->context)};
                    LLVMTypeRef ret_type = LLVMVoidTypeInContext(codegen->context);
                    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, 1, 0);
                    func = LLVMAddFunction(codegen->module, "random_seed", func_type);
                }
                result = LLVMBuildCall2(codegen->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, arg_count, "");
                // Void function, return NULL
                result = NULL;
            }
            else {
                fprintf(stderr, "Error: Unknown stdlib function '%s'\n", func_name);
                result = NULL;
            }

    // 调试：记录标准库函数调用结束
    if (codegen->debug_ctx) {
        debug_log_function_return(codegen->debug_ctx, func_name, NULL);
        debug_perf_end(codegen->debug_ctx, func_name);
    }            if (args) free(args);
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
                    
                    // 调试：记录变量初始化
                    debug_log_variable_value(codegen, name, init_value);
                    
                    // 可视化：添加变量初始化节点
                    if (codegen->debug_ctx) {
                        char label[256];
                        snprintf(label, sizeof(label), "%s = init", name);
                        debug_viz_add_node(codegen->debug_ctx, label, "box", "lightyellow");
                    }
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
                    
                    // 调试：记录变量赋值
                    debug_log_variable_value(codegen, target, value);
                    
                    // 可视化：添加变量赋值节点
                    if (codegen->debug_ctx) {
                        char label[256];
                        snprintf(label, sizeof(label), "%s = value", target);
                        debug_viz_add_node(codegen->debug_ctx, label, "box", "lightyellow");
                    }
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
        
        case NODE_PARALLEL: {
            // 处理并行语句
            ASTNode* body = node->data.parallel_stmt.body;
            
            // 分析循环体是否适合并行化
            if (is_loop_suitable_for_parallelization(body)) {
                fprintf(stderr, "Info: Loop body is suitable for OpenMP parallelization\n");
                // 生成OpenMP并行指令
                // 这里可以添加实际的OpenMP指令生成代码
            } else {
                fprintf(stderr, "Warning: Loop body contains operations that conflict with parallelization (I/O, events, etc.)\n");
                fprintf(stderr, "Warning: Generating sequential code instead of parallel\n");
            }
            
            // 无论是否并行化，都要生成循环体代码
            codegen_statement(codegen, body);
            break;
        }
        
        case NODE_PARALLEL_SECTIONS: {
            // 处理并行段语句
            fprintf(stderr, "Info: Generating OpenMP parallel sections\n");
            
            // 处理每个段
            if (node->data.parallel_sections_stmt.sections) {
                codegen_statement(codegen, node->data.parallel_sections_stmt.sections);
            }
            break;
        }
        
        case NODE_SECTION_LIST: {
            // 处理段列表
            for (int i = 0; i < node->data.list.count; i++) {
                fprintf(stderr, "Info: Generating OpenMP section %d\n", i);
                codegen_statement(codegen, node->data.list.items[i]);
            }
            break;
        }
        
        case NODE_CRITICAL: {
            // 处理临界区语句
            fprintf(stderr, "Info: Generating OpenMP critical section\n");
            
            // 生成临界区代码
            codegen_statement(codegen, node->data.critical_stmt.body);
            break;
        }
        
        case NODE_BARRIER: {
            // 处理屏障语句
            fprintf(stderr, "Info: Generating OpenMP barrier\n");
            // OpenMP屏障在这里不需要额外的代码生成
            break;
        }
        
        case NODE_MASTER: {
            // 处理主线程语句
            fprintf(stderr, "Info: Generating OpenMP master region\n");
            
            // 生成主线程区域代码
            codegen_statement(codegen, node->data.master_stmt.body);
            break;
        }
        
        case NODE_SINGLE: {
            // 处理单线程语句
            fprintf(stderr, "Info: Generating OpenMP single region\n");
            
            // 生成单线程区域代码
            codegen_statement(codegen, node->data.single_stmt.body);
            break;
        }
        
        case NODE_THREADPRIVATE: {
            // 处理线程私有语句
            const char* var_name = node->data.threadprivate_stmt.variable_name;
            fprintf(stderr, "Info: Marking variable '%s' as thread private\n", var_name);
            
            // 在符号表中标记变量为线程私有
            Symbol* symbol = symbol_table_lookup(codegen->symbol_table, var_name);
            if (symbol) {
                // 这里可以设置一个标志来表示变量是线程私有的
                // 实际的OpenMP实现需要更多的基础设施
            } else {
                fprintf(stderr, "Warning: Variable '%s' not found for threadprivate\n", var_name);
            }
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

/* ===== 目标代码生成和链接功能实现 ===== */

/* 生成目标文件 (.o) */
int codegen_emit_object_file(CodeGenerator* codegen, const char* filename) {
    if (!codegen) return 0;
    
    char* error = NULL;
    
    // 初始化目标
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    
    // 获取目标三元组
    const char* triple = LLVMGetDefaultTargetTriple();
    LLVMSetTarget(codegen->module, triple);
    
    // 创建目标机器
    LLVMTargetRef target = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Error: Failed to get target: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeMessage((char*)triple);
        return 0;
    }
    
    // 创建目标机器
    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
        target, triple, "generic", "", 
        LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);
    
    if (!target_machine) {
        fprintf(stderr, "Error: Failed to create target machine\n");
        LLVMDisposeMessage((char*)triple);
        return 0;
    }
    
    // 生成目标文件
    if (LLVMTargetMachineEmitToFile(target_machine, codegen->module, 
                                   (char*)filename, LLVMObjectFile, &error) != 0) {
        fprintf(stderr, "Error: Failed to emit object file: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeTargetMachine(target_machine);
        LLVMDisposeMessage((char*)triple);
        return 0;
    }
    
    LLVMDisposeTargetMachine(target_machine);
    LLVMDisposeMessage((char*)triple);
    return 1;
}

/* 生成可执行文件 */
int codegen_emit_executable(CodeGenerator* codegen, const char* filename) {
    if (!codegen) return 0;
    
    // 先生成临时目标文件
    const char* temp_obj = "temp_simscript.o";
    
    if (!codegen_emit_object_file(codegen, temp_obj)) {
        return 0;
    }
    
    // 使用系统链接器链接生成可执行文件
    char link_cmd[1024];
    snprintf(link_cmd, sizeof(link_cmd), "gcc -no-pie -o %s %s", filename, temp_obj);
    
    int result = system(link_cmd);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to link executable\n");
        remove(temp_obj);
        return 0;
    }
    
    // 清理临时文件
    remove(temp_obj);
    return 1;
}
