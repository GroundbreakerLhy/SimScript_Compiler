#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "frontend/ast.h"
#include "codegen/codegen.h"
#include "debug/debug.h"

extern "C" {
    extern FILE* yyin;
    extern ASTNode* root;
    extern int yyparse();
    int yycolumn = 1;
}

void print_usage(const char* prog_name) {
    printf("Usage: %s [options] <input_file>\n", prog_name);
    printf("Options:\n");
    printf("  -o <file>     Specify output file (default: output.ll or a.out)\n");
    printf("  -g            Enable basic debugging\n");
    printf("  --debug <level> Enable debugging (none, basic, detailed, verbose)\n");
    printf("  --debug-file <file> Specify debug output file (default: stdout)\n");
    printf("  --execute     Execute code using JIT instead of generating IR\n");
    printf("  --breakpoint <type> <location> Set breakpoint (type: line, function)\n");
    printf("  -c            Compile to object file instead of executable\n");
    printf("  -h, --help    Show this help message\n");
    printf("  --print-ast   Print AST\n");
    printf("  --print-ir    Print LLVM IR\n");
}

int main(int argc, char* argv[]) {
    const char* input_file = NULL;
    const char* output_file = NULL; // 默认为NULL，根据模式自动设置
    int print_ast = 0;
    int print_ir = 0;
    int execute_jit = 0;
    int compile_only = 0; // -c 选项：只编译不链接
    DebugLevel debug_level = DEBUG_LEVEL_NONE;
    const char* debug_file = NULL;
    
    // 断点信息
    struct {
        BreakpointType type;
        const char* location;
    } breakpoints[10]; // 最多10个断点
    int breakpoint_count = 0;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0) {
            compile_only = 1;
        } else if (strcmp(argv[i], "-g") == 0) {
            debug_level = DEBUG_LEVEL_BASIC;
        } else if (strcmp(argv[i], "--print-ast") == 0) {
            print_ast = 1;
        } else if (strcmp(argv[i], "--print-ir") == 0) {
            print_ir = 1;
        } else if (strcmp(argv[i], "--execute") == 0) {
            execute_jit = 1;
        } else if (strcmp(argv[i], "--breakpoint") == 0 && i + 2 < argc) {
            // 解析断点参数
            const char* type_str = argv[++i];
            const char* location = argv[++i];
            
            BreakpointType type;
            if (strcmp(type_str, "line") == 0) {
                type = BREAKPOINT_LINE;
            } else if (strcmp(type_str, "function") == 0) {
                type = BREAKPOINT_FUNCTION;
            } else {
                fprintf(stderr, "Invalid breakpoint type: %s\n", type_str);
                return 1;
            }
            
            if (breakpoint_count < 10) {
                breakpoints[breakpoint_count].type = type;
                breakpoints[breakpoint_count].location = location;
                breakpoint_count++;
            } else {
                fprintf(stderr, "Too many breakpoints (max 10)\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--debug") == 0 && i + 1 < argc) {
            const char* level_str = argv[++i];
            if (strcmp(level_str, "none") == 0) debug_level = DEBUG_LEVEL_NONE;
            else if (strcmp(level_str, "basic") == 0) debug_level = DEBUG_LEVEL_BASIC;
            else if (strcmp(level_str, "detailed") == 0) debug_level = DEBUG_LEVEL_DETAILED;
            else if (strcmp(level_str, "verbose") == 0) debug_level = DEBUG_LEVEL_VERBOSE;
            else {
                fprintf(stderr, "Invalid debug level: %s\n", level_str);
                return 1;
            }
        } else if (strcmp(argv[i], "--debug-file") == 0 && i + 1 < argc) {
            debug_file = argv[++i];
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (!input_file) {
        fprintf(stderr, "Error: Input file required\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 设置默认输出文件名
    if (!output_file) {
        if (execute_jit) {
            // JIT执行不需要输出文件
        } else if (compile_only) {
            // 编译到目标文件：将.sim扩展名改为.o
            char* default_output = (char*)malloc(strlen(input_file) + 3);
            strcpy(default_output, input_file);
            char* dot_pos = strrchr(default_output, '.');
            if (dot_pos) {
                strcpy(dot_pos, ".o");
            } else {
                strcat(default_output, ".o");
            }
            output_file = default_output;
        } else {
            // 默认生成可执行文件
            output_file = "a.out";
        }
    }
    
    // Open input file
    yyin = fopen(input_file, "r");
    if (!yyin) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        return 1;
    }
    
    
    // Parse file
    if (yyparse() != 0) {
        fprintf(stderr, "Parse failed\n");
        fclose(yyin);
        return 1;
    }
    
    fclose(yyin);
    
    if (!root) {
        fprintf(stderr, "Error: No AST generated\n");
        return 1;
    }
    
    
    // 打印 AST（如果需要）
    if (print_ast) {
        printf("\n=== AST ===\n");
        print_ast_tree(root, 0);
    }
    
    // 创建调试上下文（如果需要）
    DebugContext* debug_ctx = NULL;
    if (debug_level != DEBUG_LEVEL_NONE) {
        debug_ctx = debug_create(debug_level, debug_file);
        if (!debug_ctx) {
            fprintf(stderr, "Error: Cannot create debug context\n");
            free_ast(root);
            return 1;
        }
        
        // 初始化可视化
        if (debug_level >= DEBUG_LEVEL_BASIC) {
            // 禁用.dot文件生成
            // char graph_file[256];
            // if (debug_file) {
            //     // 移除扩展名并添加 .dot
            //     const char* dot_pos = strstr(debug_file, ".log");
            //     if (dot_pos) {
            //         size_t base_len = dot_pos - debug_file;
            //         strncpy(graph_file, debug_file, base_len);
            //         graph_file[base_len] = '\0';
            //     } else {
            //         strcpy(graph_file, debug_file);
            //     }
            //     strcat(graph_file, ".dot");
            // } else {
            //     strcpy(graph_file, "execution_flow.dot");
            // }
            // printf("Generating visualization file: %s\n", graph_file);
            // debug_viz_init(debug_ctx, graph_file);
        }
        
        // 设置断点
        for (int i = 0; i < breakpoint_count; i++) {
            int bp_id = debug_set_breakpoint(debug_ctx, breakpoints[i].type, breakpoints[i].location);
            if (bp_id < 0) {
                fprintf(stderr, "Error: Failed to set breakpoint at %s\n", breakpoints[i].location);
            } else {
                printf("Set breakpoint %d at %s\n", bp_id, breakpoints[i].location);
            }
        }
    }
    
    // Generate code
    CodeGenerator* codegen = codegen_create_with_debug("SimScript", debug_ctx);
    if (!codegen) {
        fprintf(stderr, "Error: Cannot create code generator\n");
        if (debug_ctx) debug_destroy(debug_ctx);
        free_ast(root);
        return 1;
    }

    if (!codegen_generate(codegen, root)) {
        fprintf(stderr, "Code generation failed\n");
        codegen_destroy(codegen);
        free_ast(root);
        return 1;
    }

    // Print IR if requested
    if (print_ir) {
        printf("\n=== LLVM IR ===\n");
        codegen_print_ir(codegen);
    }

    if (execute_jit) {
        // 使用JIT执行代码
        if (!codegen_init_jit(codegen)) {
            fprintf(stderr, "Failed to initialize JIT\n");
            codegen_destroy(codegen);
            free_ast(root);
            if (debug_ctx) debug_destroy(debug_ctx);
            return 1;
        }

        if (!codegen_execute_jit(codegen)) {
            fprintf(stderr, "JIT execution failed\n");
            codegen_destroy_jit(codegen);
            codegen_destroy(codegen);
            free_ast(root);
            if (debug_ctx) debug_destroy(debug_ctx);
            return 1;
        }

        // 清理JIT
        codegen_destroy_jit(codegen);
        
        // 清理AST
        free_ast(root);
        
        // 清理调试上下文
        if (debug_ctx) {
            // debug_viz_finish(debug_ctx); // 禁用.dot文件生成
            debug_destroy(debug_ctx);
        }
        
        // 注意：不清理codegen以避免段错误
        // codegen_destroy(codegen);
        
        return 0;
    } else {
        // 根据选项生成不同的输出
        if (compile_only) {
            // 只编译到目标文件
            if (!codegen_emit_object_file(codegen, output_file)) {
                fprintf(stderr, "Failed to generate object file\n");
                codegen_destroy(codegen);
                free_ast(root);
                if (debug_ctx) debug_destroy(debug_ctx);
                return 1;
            }
        } else {
            // 生成可执行文件
            if (!codegen_emit_executable(codegen, output_file)) {
                fprintf(stderr, "Failed to generate executable\n");
                codegen_destroy(codegen);
                free_ast(root);
                if (debug_ctx) debug_destroy(debug_ctx);
                return 1;
            }
        }

    }
    
    // 清理
    codegen_destroy(codegen);
    free_ast(root);
    if (debug_ctx) {
        // debug_viz_finish(debug_ctx); // 禁用.dot文件生成
        debug_destroy(debug_ctx);
    }
    
    return 0;
}
