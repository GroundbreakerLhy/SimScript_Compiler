#pragma once

#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

// 设置全局调试上下文
void simscript_debug_set_context(DebugContext* ctx);

// 调试钩子函数：检查断点
void simscript_debug_hook(int line_number, const char* function_name);

#ifdef __cplusplus
}
#endif