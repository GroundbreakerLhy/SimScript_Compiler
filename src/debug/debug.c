#include "debug.h"
#include <stdarg.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* 创建调试上下文 */
DebugContext* debug_create(DebugLevel level, const char* output_file) {
    DebugContext* ctx = (DebugContext*)malloc(sizeof(DebugContext));
    if (!ctx) return NULL;

    ctx->level = level;
    ctx->indent_level = 0;
    ctx->enabled = 1;
    ctx->start_time = clock();
    ctx->perf_start_time = 0.0;
    ctx->perf_enabled = (level >= DEBUG_LEVEL_DETAILED);
    ctx->graph_file = NULL;
    ctx->node_counter = 0;
    ctx->viz_enabled = (level >= DEBUG_LEVEL_BASIC);

    // 初始化断点调试相关字段
    ctx->breakpoints = NULL;
    ctx->next_breakpoint_id = 1;
    ctx->debugger_state = DEBUGGER_RUNNING;
    ctx->current_line = 0;
    ctx->current_function = NULL;
    ctx->stack_frame = NULL;

    if (output_file) {
        ctx->output_file = fopen(output_file, "w");
        if (!ctx->output_file) {
            free(ctx);
            return NULL;
        }
    } else {
        ctx->output_file = stdout;
    }

    return ctx;
}

/* 销毁调试上下文 */
void debug_destroy(DebugContext* ctx) {
    if (!ctx) return;

    if (ctx->output_file && ctx->output_file != stdout) {
        fclose(ctx->output_file);
    }

    // 清理断点列表
    Breakpoint* bp = ctx->breakpoints;
    while (bp) {
        Breakpoint* next = bp->next;
        free(bp->location);
        free(bp);
        bp = next;
    }

    // 清理当前函数名
    if (ctx->current_function) {
        free(ctx->current_function);
    }

    free(ctx);
}

/* 启用/禁用调试 */
void debug_enable(DebugContext* ctx, int enabled) {
    if (ctx) {
        ctx->enabled = enabled;
    }
}

/* 设置调试级别 */
void debug_set_level(DebugContext* ctx, DebugLevel level) {
    if (ctx) {
        ctx->level = level;
    }
}

/* 获取当前时间戳 */
double debug_get_timestamp(void) {
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

/* 记录调试信息 */
void debug_log(DebugContext* ctx, DebugInfoType type, const char* format, ...) {
    if (!ctx || !ctx->enabled) return;

    // 根据调试级别过滤
    if (ctx->level == DEBUG_LEVEL_NONE) return;

    // 某些类型的信息只在详细级别显示
    if ((type == DEBUG_INFO_MEMORY || type == DEBUG_INFO_PERFORMANCE) &&
        ctx->level < DEBUG_LEVEL_DETAILED) return;

    // 输出时间戳
    double timestamp = debug_get_timestamp();
    fprintf(ctx->output_file, "[%.3f] ", timestamp);

    // 输出缩进
    for (int i = 0; i < ctx->indent_level; i++) {
        fprintf(ctx->output_file, "  ");
    }

    // 输出类型标识
    const char* type_str = "";
    switch (type) {
        case DEBUG_INFO_VARIABLE: type_str = "VAR"; break;
        case DEBUG_INFO_FUNCTION_CALL: type_str = "CALL"; break;
        case DEBUG_INFO_FUNCTION_RETURN: type_str = "RET"; break;
        case DEBUG_INFO_LOOP_START: type_str = "LOOP_START"; break;
        case DEBUG_INFO_LOOP_END: type_str = "LOOP_END"; break;
        case DEBUG_INFO_CONDITION: type_str = "COND"; break;
        case DEBUG_INFO_MEMORY: type_str = "MEM"; break;
        case DEBUG_INFO_PERFORMANCE: type_str = "PERF"; break;
        case DEBUG_INFO_BREAKPOINT: type_str = "BP"; break;
    }

    fprintf(ctx->output_file, "%s: ", type_str);

    // 输出格式化消息
    va_list args;
    va_start(args, format);
    vfprintf(ctx->output_file, format, args);
    va_end(args);

    fprintf(ctx->output_file, "\n");
    fflush(ctx->output_file);
}

/* 记录变量值 */
void debug_log_variable(DebugContext* ctx, const char* name, const char* type, const char* value) {
    debug_log(ctx, DEBUG_INFO_VARIABLE, "%s (%s) = %s", name, type, value);
}

/* 记录函数调用 */
void debug_log_function_call(DebugContext* ctx, const char* function_name, int arg_count, ...) {
    if (!ctx || !ctx->enabled || ctx->level < DEBUG_LEVEL_BASIC) return;

    fprintf(ctx->output_file, "[%.3f] ", debug_get_timestamp());
    for (int i = 0; i < ctx->indent_level; i++) {
        fprintf(ctx->output_file, "  ");
    }
    fprintf(ctx->output_file, "CALL: %s(", function_name);

    va_list args;
    va_start(args, arg_count);
    for (int i = 0; i < arg_count; i++) {
        if (i > 0) fprintf(ctx->output_file, ", ");
        fprintf(ctx->output_file, "%s", va_arg(args, const char*));
    }
    va_end(args);

    fprintf(ctx->output_file, ")\n");
    fflush(ctx->output_file);

    debug_indent_increase(ctx);
}

/* 记录函数返回 */
void debug_log_function_return(DebugContext* ctx, const char* function_name, const char* return_value) {
    if (!ctx || !ctx->enabled || ctx->level < DEBUG_LEVEL_BASIC) return;

    debug_indent_decrease(ctx);

    fprintf(ctx->output_file, "[%.3f] ", debug_get_timestamp());
    for (int i = 0; i < ctx->indent_level; i++) {
        fprintf(ctx->output_file, "  ");
    }
    fprintf(ctx->output_file, "RET: %s -> %s\n", function_name, return_value ? return_value : "void");
    fflush(ctx->output_file);
}

/* 记录循环开始 */
void debug_log_loop_start(DebugContext* ctx, const char* loop_type, int iteration) {
    debug_log(ctx, DEBUG_INFO_LOOP_START, "%s loop iteration %d", loop_type, iteration);
    debug_indent_increase(ctx);
}

/* 记录循环结束 */
void debug_log_loop_end(DebugContext* ctx, const char* loop_type) {
    debug_indent_decrease(ctx);
    debug_log(ctx, DEBUG_INFO_LOOP_END, "%s loop ended", loop_type);
}

/* 记录条件判断 */
void debug_log_condition(DebugContext* ctx, const char* condition, int result) {
    debug_log(ctx, DEBUG_INFO_CONDITION, "%s -> %s", condition, result ? "true" : "false");
}

/* 记录内存使用 */
void debug_log_memory(DebugContext* ctx, size_t allocated, size_t freed) {
    if (allocated > 0) {
        debug_log(ctx, DEBUG_INFO_MEMORY, "allocated %zu bytes", allocated);
    }
    if (freed > 0) {
        debug_log(ctx, DEBUG_INFO_MEMORY, "freed %zu bytes", freed);
    }
}

/* 记录性能信息 */
void debug_log_performance(DebugContext* ctx, const char* operation, double time_ms) {
    debug_log(ctx, DEBUG_INFO_PERFORMANCE, "%s took %.3f ms", operation, time_ms);
}

/* 开始性能计时 */
void debug_perf_start(DebugContext* ctx, const char* operation) {
    if (!ctx || !ctx->perf_enabled) return;
    
    ctx->perf_start_time = debug_get_timestamp();
    debug_log(ctx, DEBUG_INFO_PERFORMANCE, "Started: %s", operation);
}

/* 结束性能计时 */
void debug_perf_end(DebugContext* ctx, const char* operation) {
    if (!ctx || !ctx->perf_enabled || ctx->perf_start_time == 0.0) return;
    
    double end_time = debug_get_timestamp();
    double duration = end_time - ctx->perf_start_time;
    debug_log_performance(ctx, operation, duration);
    ctx->perf_start_time = 0.0;
}

/* 初始化可视化图 */
void debug_viz_init(DebugContext* ctx, const char* graph_file) {
    if (!ctx || !ctx->viz_enabled) return;
    
    if (graph_file) {
        ctx->graph_file = fopen(graph_file, "w");
        if (ctx->graph_file) {
            fprintf(ctx->graph_file, "digraph ExecutionFlow {\n");
            fprintf(ctx->graph_file, "  rankdir=TB;\n");
            fprintf(ctx->graph_file, "  node [shape=box, style=filled, fillcolor=lightblue];\n");
            fprintf(ctx->graph_file, "\n");
        }
    }
}

/* 完成可视化图 */
void debug_viz_finish(DebugContext* ctx) {
    if (!ctx || !ctx->graph_file) return;
    
    fprintf(ctx->graph_file, "}\n");
    fclose(ctx->graph_file);
    ctx->graph_file = NULL;
}

/* 添加可视化节点 */
int debug_viz_add_node(DebugContext* ctx, const char* label, const char* shape, const char* color) {
    if (!ctx || !ctx->graph_file) return -1;
    
    int node_id = ctx->node_counter++;
    fprintf(ctx->graph_file, "  node%d [label=\"%s\", shape=%s, fillcolor=%s];\n", 
            node_id, label, shape, color);
    return node_id;
}

/* 添加可视化边 */
void debug_viz_add_edge(DebugContext* ctx, int from_node, int to_node, const char* label) {
    if (!ctx || !ctx->graph_file) return;
    
    if (label) {
        fprintf(ctx->graph_file, "  node%d -> node%d [label=\"%s\"];\n", from_node, to_node, label);
    } else {
        fprintf(ctx->graph_file, "  node%d -> node%d;\n", from_node, to_node);
    }
}

/* 增加缩进 */
void debug_indent_increase(DebugContext* ctx) {
    if (ctx) ctx->indent_level++;
}

/* 减少缩进 */
void debug_indent_decrease(DebugContext* ctx) {
    if (ctx) ctx->indent_level = ctx->indent_level > 0 ? ctx->indent_level - 1 : 0;
}

/* ===== 断点调试功能实现 ===== */

/* 设置断点 */
int debug_set_breakpoint(DebugContext* ctx, BreakpointType type, const char* location) {
    if (!ctx) return -1;

    Breakpoint* bp = (Breakpoint*)malloc(sizeof(Breakpoint));
    if (!bp) return -1;

    bp->id = ctx->next_breakpoint_id++;
    bp->type = type;
    bp->location = strdup(location);
    bp->enabled = 1;
    bp->hit_count = 0;
    bp->next = ctx->breakpoints;

    ctx->breakpoints = bp;

    debug_log(ctx, DEBUG_INFO_BREAKPOINT, "Set breakpoint %d at %s", bp->id, location);
    return bp->id;
}

/* 删除断点 */
int debug_remove_breakpoint(DebugContext* ctx, int breakpoint_id) {
    if (!ctx) return 0;

    Breakpoint* prev = NULL;
    Breakpoint* curr = ctx->breakpoints;

    while (curr) {
        if (curr->id == breakpoint_id) {
            if (prev) {
                prev->next = curr->next;
            } else {
                ctx->breakpoints = curr->next;
            }
            free(curr->location);
            free(curr);
            debug_log(ctx, DEBUG_INFO_BREAKPOINT, "Removed breakpoint %d", breakpoint_id);
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }

    return 0;
}

/* 启用/禁用断点 */
int debug_enable_breakpoint(DebugContext* ctx, int breakpoint_id, int enabled) {
    if (!ctx) return 0;

    Breakpoint* bp = ctx->breakpoints;
    while (bp) {
        if (bp->id == breakpoint_id) {
            bp->enabled = enabled;
            debug_log(ctx, DEBUG_INFO_BREAKPOINT, "%s breakpoint %d",
                     enabled ? "Enabled" : "Disabled", breakpoint_id);
            return 1;
        }
        bp = bp->next;
    }

    return 0;
}

/* 检查是否命中断点 */
int debug_check_breakpoint(DebugContext* ctx, BreakpointType type, const char* location) {
    if (!ctx || ctx->debugger_state == DEBUGGER_FINISHED) return 0;

    Breakpoint* bp = ctx->breakpoints;
    while (bp) {
        if (bp->enabled && bp->type == type) {
            int match = 0;
            switch (type) {
                case BREAKPOINT_LINE:
                    match = (atoi(location) == ctx->current_line);
                    break;
                case BREAKPOINT_FUNCTION:
                    match = (ctx->current_function && strcmp(location, ctx->current_function) == 0);
                    break;
                case BREAKPOINT_CONDITION:
                    // 简单的条件检查，这里可以扩展为表达式求值
                    match = (strcmp(location, "true") == 0);
                    break;
            }

            if (match) {
                bp->hit_count++;
                debug_log_breakpoint_hit(ctx, bp->id, location);
                ctx->debugger_state = DEBUGGER_BREAK;
                return bp->id;
            }
        }
        bp = bp->next;
    }

    return 0;
}

/* 获取断点列表 */
Breakpoint* debug_get_breakpoints(DebugContext* ctx) {
    return ctx ? ctx->breakpoints : NULL;
}

/* 设置调试器状态 */
void debug_set_debugger_state(DebugContext* ctx, DebuggerState state) {
    if (ctx) {
        ctx->debugger_state = state;
        debug_log(ctx, DEBUG_INFO_BREAKPOINT, "Debugger state changed to %d", state);
    }
}

/* 获取调试器状态 */
DebuggerState debug_get_debugger_state(DebugContext* ctx) {
    return ctx ? ctx->debugger_state : DEBUGGER_FINISHED;
}

/* 记录断点命中 */
void debug_log_breakpoint_hit(DebugContext* ctx, int breakpoint_id, const char* location) {
    debug_log(ctx, DEBUG_INFO_BREAKPOINT, "Breakpoint %d hit at %s (hit count: %d)",
             breakpoint_id, location, ctx->breakpoints ?
             ((Breakpoint*)ctx->breakpoints)->hit_count : 0);
}

/* 调试器命令处理 */
int debug_process_command(DebugContext* ctx, const char* command) {
    if (!ctx || !command) return 0;

    char cmd[256];
    strcpy(cmd, command);

    // 简单的命令解析
    if (strcmp(cmd, "continue") == 0 || strcmp(cmd, "c") == 0) {
        debug_continue(ctx);
        return 1;
    } else if (strcmp(cmd, "step") == 0 || strcmp(cmd, "s") == 0) {
        debug_step(ctx);
        return 1;
    } else if (strcmp(cmd, "breakpoints") == 0 || strcmp(cmd, "b") == 0) {
        Breakpoint* bp = ctx->breakpoints;
        fprintf(ctx->output_file, "Breakpoints:\n");
        while (bp) {
            fprintf(ctx->output_file, "  %d: %s at %s (%s, hits: %d)\n",
                   bp->id,
                   bp->type == BREAKPOINT_LINE ? "line" :
                   bp->type == BREAKPOINT_FUNCTION ? "function" : "condition",
                   bp->location,
                   bp->enabled ? "enabled" : "disabled",
                   bp->hit_count);
            bp = bp->next;
        }
        return 1;
    } else if (strcmp(cmd, "variables") == 0 || strcmp(cmd, "v") == 0) {
        debug_list_variables(ctx);
        return 1;
    } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
        ctx->debugger_state = DEBUGGER_FINISHED;
        return 1;
    } else if (strncmp(cmd, "print ", 6) == 0) {
        debug_inspect_variable(ctx, cmd + 6);
        return 1;
    } else if (strncmp(cmd, "memory ", 7) == 0) {
        // 简单的内存检查，格式: memory <address> <size>
        void* addr = NULL;
        size_t size = 16;
        sscanf(cmd + 7, "%p %zu", &addr, &size);
        debug_inspect_memory(ctx, addr, size);
        return 1;
    }

    fprintf(ctx->output_file, "Unknown command: %s\n", cmd);
    fprintf(ctx->output_file, "Available commands: continue(c), step(s), breakpoints(b), variables(v), print <var>, memory <addr> <size>, quit(q)\n");
    return 0;
}

/* 查看变量 */
void debug_inspect_variable(DebugContext* ctx, const char* var_name) {
    if (!ctx) return;

    // 这里需要与符号表集成来查找变量
    // 暂时显示占位符信息
    fprintf(ctx->output_file, "Variable %s: <not implemented - needs symbol table integration>\n", var_name);
}

/* 查看内存 */
void debug_inspect_memory(DebugContext* ctx, void* address, size_t size) {
    if (!ctx || !address) return;

    fprintf(ctx->output_file, "Memory at %p:\n", address);

    unsigned char* ptr = (unsigned char*)address;
    for (size_t i = 0; i < size; i += 16) {
        fprintf(ctx->output_file, "  %p: ", ptr + i);

        // 十六进制显示
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            fprintf(ctx->output_file, "%02x ", ptr[i + j]);
        }

        // ASCII显示
        fprintf(ctx->output_file, " |");
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            char c = ptr[i + j];
            fprintf(ctx->output_file, "%c", (c >= 32 && c <= 126) ? c : '.');
        }
        fprintf(ctx->output_file, "|\n");
    }
}

/* 列出当前作用域的变量 */
void debug_list_variables(DebugContext* ctx) {
    if (!ctx) return;

    fprintf(ctx->output_file, "Local variables: <not implemented - needs symbol table integration>\n");
    fprintf(ctx->output_file, "Current function: %s\n", ctx->current_function ? ctx->current_function : "<none>");
    fprintf(ctx->output_file, "Current line: %d\n", ctx->current_line);
}

/* 单步执行 */
void debug_step(DebugContext* ctx) {
    if (ctx) {
        ctx->debugger_state = DEBUGGER_STEP;
        debug_log(ctx, DEBUG_INFO_BREAKPOINT, "Stepping to next instruction");
    }
}

/* 继续执行 */
void debug_continue(DebugContext* ctx) {
    if (ctx) {
        ctx->debugger_state = DEBUGGER_RUNNING;
        debug_log(ctx, DEBUG_INFO_BREAKPOINT, "Continuing execution");
    }
}

/* 设置当前执行位置 */
void debug_set_location(DebugContext* ctx, int line, const char* function) {
    if (!ctx) return;

    ctx->current_line = line;

    if (ctx->current_function) {
        free(ctx->current_function);
    }
    ctx->current_function = function ? strdup(function) : NULL;

    // 检查断点
    char line_str[32];
    sprintf(line_str, "%d", line);
    debug_check_breakpoint(ctx, BREAKPOINT_LINE, line_str);

    if (function) {
        debug_check_breakpoint(ctx, BREAKPOINT_FUNCTION, function);
    }
}