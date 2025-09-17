#pragma once

#include "../frontend/ast.h"
#include "../debug/debug.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 代码生成器结构 */
typedef struct CodeGenerator CodeGenerator;

/* 创建代码生成器 */
CodeGenerator* codegen_create(const char* module_name);

/* 创建带调试功能的代码生成器 */
CodeGenerator* codegen_create_with_debug(const char* module_name, DebugContext* debug_ctx);

/* 销毁代码生成器 */
void codegen_destroy(CodeGenerator* codegen);

/* 生成代码 */
int codegen_generate(CodeGenerator* codegen, ASTNode* ast);

/* 输出 LLVM IR 到文件 */
int codegen_write_to_file(CodeGenerator* codegen, const char* filename);

/* 输出 LLVM IR 到标准输出 */
void codegen_print_ir(CodeGenerator* codegen);

/* 获取调试上下文 */
DebugContext* codegen_get_debug_context(CodeGenerator* codegen);

/* 设置调试上下文 */
void codegen_set_debug_context(CodeGenerator* codegen, DebugContext* debug_ctx);

/* ===== JIT 执行和调试功能 ===== */

/* 初始化JIT执行引擎 */
int codegen_init_jit(CodeGenerator* codegen);

/* 执行生成的代码（使用JIT） */
int codegen_execute_jit(CodeGenerator* codegen);

/* 销毁JIT执行引擎 */
void codegen_destroy_jit(CodeGenerator* codegen);

/* 检查是否支持JIT调试 */
int codegen_supports_debug_execution(CodeGenerator* codegen);

/* ===== 目标代码生成和链接功能 ===== */

/* 生成目标文件 (.o) */
int codegen_emit_object_file(CodeGenerator* codegen, const char* filename);

/* 生成可执行文件 */
int codegen_emit_executable(CodeGenerator* codegen, const char* filename);

#ifdef __cplusplus
}
#endif
