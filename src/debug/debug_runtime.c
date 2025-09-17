#include "debug.h"
#include <stdio.h>

// 全局调试上下文指针（在JIT执行时使用）
static DebugContext* global_debug_ctx = NULL;

// 设置全局调试上下文
void simscript_debug_set_context(DebugContext* ctx) {
    global_debug_ctx = ctx;
}

// 调试钩子函数：检查断点
void simscript_debug_hook(int line_number, const char* function_name) {
    if (!global_debug_ctx) {
        printf("Warning: Debug context not available\n");
        return;
    }

    // 检查调试上下文是否仍然有效
    if (global_debug_ctx->enabled == 0) {
        return; // 上下文已被禁用
    }

    // 设置当前位置
    debug_set_location(global_debug_ctx, line_number, function_name);

    // 检查调试器状态
    DebuggerState state = debug_get_debugger_state(global_debug_ctx);
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
            } else if (debug_process_command(global_debug_ctx, command)) {
                if (debug_get_debugger_state(global_debug_ctx) == DEBUGGER_FINISHED) {
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