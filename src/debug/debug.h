#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 调试级别 */
typedef enum {
    DEBUG_LEVEL_NONE = 0,
    DEBUG_LEVEL_BASIC = 1,
    DEBUG_LEVEL_DETAILED = 2,
    DEBUG_LEVEL_VERBOSE = 3
} DebugLevel;

/* 调试信息类型 */
typedef enum {
    DEBUG_INFO_VARIABLE = 1,
    DEBUG_INFO_FUNCTION_CALL = 2,
    DEBUG_INFO_FUNCTION_RETURN = 3,
    DEBUG_INFO_LOOP_START = 4,
    DEBUG_INFO_LOOP_END = 5,
    DEBUG_INFO_CONDITION = 6,
    DEBUG_INFO_MEMORY = 7,
    DEBUG_INFO_PERFORMANCE = 8,
    DEBUG_INFO_BREAKPOINT = 9
} DebugInfoType;

/* 断点类型 */
typedef enum {
    BREAKPOINT_LINE = 1,      /* 行号断点 */
    BREAKPOINT_FUNCTION = 2,  /* 函数断点 */
    BREAKPOINT_CONDITION = 3  /* 条件断点 */
} BreakpointType;

/* 断点结构 */
typedef struct Breakpoint {
    int id;                    /* 断点ID */
    BreakpointType type;       /* 断点类型 */
    char* location;            /* 位置（行号、函数名或条件表达式） */
    int enabled;               /* 是否启用 */
    int hit_count;             /* 命中次数 */
    struct Breakpoint* next;   /* 下一个断点 */
} Breakpoint;

/* 调试器状态 */
typedef enum {
    DEBUGGER_RUNNING = 0,      /* 运行中 */
    DEBUGGER_STOPPED = 1,      /* 停止 */
    DEBUGGER_BREAK = 2,        /* 断点停止 */
    DEBUGGER_STEP = 3,         /* 单步执行 */
    DEBUGGER_FINISHED = 4      /* 执行完毕 */
} DebuggerState;

/* 变量信息 */
typedef struct VariableInfo {
    char* name;                /* 变量名 */
    char* type;                /* 类型 */
    void* address;             /* 内存地址 */
    union {
        int int_val;
        double double_val;
        char* string_val;
    } value;                   /* 值 */
} VariableInfo;

/* 调试上下文 */
typedef struct DebugContext {
    DebugLevel level;
    FILE* output_file;
    int indent_level;
    clock_t start_time;
    int enabled;
    // 性能分析相关
    double perf_start_time;
    int perf_enabled;
    // 可视化相关
    FILE* graph_file;
    int node_counter;
    int viz_enabled;
    // 断点调试相关
    Breakpoint* breakpoints;           /* 断点列表 */
    int next_breakpoint_id;            /* 下一个断点ID */
    DebuggerState debugger_state;      /* 调试器状态 */
    int current_line;                  /* 当前执行行号 */
    char* current_function;            /* 当前函数名 */
    void* stack_frame;                 /* 当前栈帧 */
} DebugContext;

/* 创建调试上下文 */
DebugContext* debug_create(DebugLevel level, const char* output_file);

/* 销毁调试上下文 */
void debug_destroy(DebugContext* ctx);

/* 启用/禁用调试 */
void debug_enable(DebugContext* ctx, int enabled);

/* 设置调试级别 */
void debug_set_level(DebugContext* ctx, DebugLevel level);

/* 记录调试信息 */
void debug_log(DebugContext* ctx, DebugInfoType type, const char* format, ...);

/* 记录变量值 */
void debug_log_variable(DebugContext* ctx, const char* name, const char* type, const char* value);

/* 记录函数调用 */
void debug_log_function_call(DebugContext* ctx, const char* function_name, int arg_count, ...);

/* 记录函数返回 */
void debug_log_function_return(DebugContext* ctx, const char* function_name, const char* return_value);

/* 记录循环开始 */
void debug_log_loop_start(DebugContext* ctx, const char* loop_type, int iteration);

/* 记录循环结束 */
void debug_log_loop_end(DebugContext* ctx, const char* loop_type);

/* 记录条件判断 */
void debug_log_condition(DebugContext* ctx, const char* condition, int result);

/* 记录内存使用 */
void debug_log_memory(DebugContext* ctx, size_t allocated, size_t freed);

/* 记录性能信息 */
void debug_log_performance(DebugContext* ctx, const char* operation, double time_ms);

/* 开始性能计时 */
void debug_perf_start(DebugContext* ctx, const char* operation);

/* 结束性能计时 */
void debug_perf_end(DebugContext* ctx, const char* operation);

/* 初始化可视化图 */
void debug_viz_init(DebugContext* ctx, const char* graph_file);

/* 完成可视化图 */
void debug_viz_finish(DebugContext* ctx);

/* 添加可视化节点 */
int debug_viz_add_node(DebugContext* ctx, const char* label, const char* shape, const char* color);

/* 添加可视化边 */
void debug_viz_add_edge(DebugContext* ctx, int from_node, int to_node, const char* label);

/* 增加缩进 */
void debug_indent_increase(DebugContext* ctx);

/* 减少缩进 */
void debug_indent_decrease(DebugContext* ctx);

/* 获取当前时间戳 */
double debug_get_timestamp(void);

/* ===== 断点调试功能 ===== */

/* 设置断点 */
int debug_set_breakpoint(DebugContext* ctx, BreakpointType type, const char* location);

/* 删除断点 */
int debug_remove_breakpoint(DebugContext* ctx, int breakpoint_id);

/* 启用/禁用断点 */
int debug_enable_breakpoint(DebugContext* ctx, int breakpoint_id, int enabled);

/* 检查是否命中断点 */
int debug_check_breakpoint(DebugContext* ctx, BreakpointType type, const char* location);

/* 获取断点列表 */
Breakpoint* debug_get_breakpoints(DebugContext* ctx);

/* 设置调试器状态 */
void debug_set_debugger_state(DebugContext* ctx, DebuggerState state);

/* 获取调试器状态 */
DebuggerState debug_get_debugger_state(DebugContext* ctx);

/* 记录断点命中 */
void debug_log_breakpoint_hit(DebugContext* ctx, int breakpoint_id, const char* location);

/* 调试器命令处理 */
int debug_process_command(DebugContext* ctx, const char* command);

/* 查看变量 */
void debug_inspect_variable(DebugContext* ctx, const char* var_name);

/* 查看内存 */
void debug_inspect_memory(DebugContext* ctx, void* address, size_t size);

/* 列出当前作用域的变量 */
void debug_list_variables(DebugContext* ctx);

/* 单步执行 */
void debug_step(DebugContext* ctx);

/* 继续执行 */
void debug_continue(DebugContext* ctx);

/* 设置当前执行位置 */
void debug_set_location(DebugContext* ctx, int line, const char* function);

#ifdef __cplusplus
}
#endif