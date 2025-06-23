#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frontend/ast.h"
#include "codegen/codegen.h"

extern "C" {
    extern FILE* yyin;
    extern ASTNode* root;
    extern int yyparse();
    int yycolumn = 1;
}

void print_usage(const char* prog_name) {
    printf("Usage: %s [options] <input_file>\n", prog_name);
    printf("Options:\n");
    printf("  -o <file>     Specify output file (default: output.ll)\n");
    printf("  -h, --help    Show this help message\n");
    printf("  --print-ast   Print AST\n");
    printf("  --print-ir    Print LLVM IR\n");
}

int main(int argc, char* argv[]) {
    const char* input_file = NULL;
    const char* output_file = "output.ll";
    int print_ast = 0;
    int print_ir = 0;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "--print-ast") == 0) {
            print_ast = 1;
        } else if (strcmp(argv[i], "--print-ir") == 0) {
            print_ir = 1;
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
    
    // Open input file
    yyin = fopen(input_file, "r");
    if (!yyin) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        return 1;
    }
    
    printf("Compiling file: %s\n", input_file);
    
    // Parse file
    printf("Parsing...\n");
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
    
    printf("Parse complete\n");
    
    // 打印 AST（如果需要）
    if (print_ast) {
        printf("\n=== AST ===\n");
        print_ast_tree(root, 0);
    }
     // Generate code
    printf("Generating LLVM IR...\n");
    CodeGenerator* codegen = codegen_create("SimScript");
    if (!codegen) {
        fprintf(stderr, "Error: Cannot create code generator\n");
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

    // Write output file
    printf("Writing output file: %s\n", output_file);
    if (!codegen_write_to_file(codegen, output_file)) {
        fprintf(stderr, "Failed to write output file\n");
        codegen_destroy(codegen);
        free_ast(root);
        return 1;
    }

    printf("Compilation complete!\n");
    printf("Output file: %s\n", output_file);
    
    // 清理
    codegen_destroy(codegen);
    free_ast(root);
    
    return 0;
}
