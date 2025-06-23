#pragma once

#include "../frontend/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 代码生成器结构 */
typedef struct CodeGenerator CodeGenerator;

/* 创建代码生成器 */
CodeGenerator* codegen_create(const char* module_name);

/* 销毁代码生成器 */
void codegen_destroy(CodeGenerator* codegen);

/* 生成代码 */
int codegen_generate(CodeGenerator* codegen, ASTNode* ast);

/* 输出 LLVM IR 到文件 */
int codegen_write_to_file(CodeGenerator* codegen, const char* filename);

/* 输出 LLVM IR 到标准输出 */
void codegen_print_ir(CodeGenerator* codegen);

#ifdef __cplusplus
}
#endif
