#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 图形输出类型 */
typedef enum {
    GRAPH_TYPE_BAR = 1,
    GRAPH_TYPE_LINE = 2,
    GRAPH_TYPE_SCATTER = 3,
    GRAPH_TYPE_HISTOGRAM = 4
} GraphType;

/* 图形数据点 */
typedef struct {
    double x;
    double y;
    const char* label;
} GraphDataPoint;

/* 图形上下文 */
typedef struct {
    GraphType type;
    const char* title;
    const char* x_label;
    const char* y_label;
    GraphDataPoint* data;
    int data_count;
    int max_data_points;
} GraphContext;

/* 创建图形上下文 */
GraphContext* graph_create(GraphType type, const char* title, const char* x_label, const char* y_label, int max_points);

/* 销毁图形上下文 */
void graph_destroy(GraphContext* ctx);

/* 添加数据点 */
int graph_add_data_point(GraphContext* ctx, double x, double y, const char* label);

/* 生成图形文件 */
int graph_generate_file(GraphContext* ctx, const char* filename);

/* 生成简单的文本图形 */
void graph_generate_text(GraphContext* ctx, FILE* output);

/* 生成统计摘要 */
void graph_generate_stats(GraphContext* ctx, FILE* output);

#ifdef __cplusplus
}
#endif