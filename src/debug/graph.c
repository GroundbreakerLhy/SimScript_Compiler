#include "graph.h"
#include <string.h>
#include <math.h>

/* 创建图形上下文 */
GraphContext* graph_create(GraphType type, const char* title, const char* x_label, const char* y_label, int max_points) {
    GraphContext* ctx = (GraphContext*)malloc(sizeof(GraphContext));
    if (!ctx) return NULL;

    ctx->type = type;
    ctx->title = title ? strdup(title) : NULL;
    ctx->x_label = x_label ? strdup(x_label) : NULL;
    ctx->y_label = y_label ? strdup(y_label) : NULL;
    ctx->data_count = 0;
    ctx->max_data_points = max_points;

    if (max_points > 0) {
        ctx->data = (GraphDataPoint*)malloc(sizeof(GraphDataPoint) * max_points);
        if (!ctx->data) {
            free(ctx);
            return NULL;
        }
    } else {
        ctx->data = NULL;
    }

    return ctx;
}

/* 销毁图形上下文 */
void graph_destroy(GraphContext* ctx) {
    if (!ctx) return;

    if (ctx->title) free((void*)ctx->title);
    if (ctx->x_label) free((void*)ctx->x_label);
    if (ctx->y_label) free((void*)ctx->y_label);
    if (ctx->data) free(ctx->data);

    free(ctx);
}

/* 添加数据点 */
int graph_add_data_point(GraphContext* ctx, double x, double y, const char* label) {
    if (!ctx || ctx->data_count >= ctx->max_data_points) {
        return -1;
    }

    ctx->data[ctx->data_count].x = x;
    ctx->data[ctx->data_count].y = y;
    ctx->data[ctx->data_count].label = label ? strdup(label) : NULL;
    ctx->data_count++;

    return ctx->data_count - 1;
}

/* 生成简单的文本图形 */
void graph_generate_text(GraphContext* ctx, FILE* output) {
    if (!ctx || !output) return;

    fprintf(output, "\n=== %s ===\n", ctx->title ? ctx->title : "Graph");
    if (ctx->x_label) fprintf(output, "X: %s\n", ctx->x_label);
    if (ctx->y_label) fprintf(output, "Y: %s\n", ctx->y_label);

    fprintf(output, "\nData Points:\n");
    for (int i = 0; i < ctx->data_count; i++) {
        fprintf(output, "  (%g, %g)", ctx->data[i].x, ctx->data[i].y);
        if (ctx->data[i].label) {
            fprintf(output, " - %s", ctx->data[i].label);
        }
        fprintf(output, "\n");
    }

    // 生成简单的 ASCII 图形
    if (ctx->data_count > 0) {
        fprintf(output, "\nASCII Graph:\n");

        // 找到数据范围
        double min_x = ctx->data[0].x, max_x = ctx->data[0].x;
        double min_y = ctx->data[0].y, max_y = ctx->data[0].y;

        for (int i = 1; i < ctx->data_count; i++) {
            if (ctx->data[i].x < min_x) min_x = ctx->data[i].x;
            if (ctx->data[i].x > max_x) max_x = ctx->data[i].x;
            if (ctx->data[i].y < min_y) min_y = ctx->data[i].y;
            if (ctx->data[i].y > max_y) max_y = ctx->data[i].y;
        }

        const int width = 40;
        const int height = 20;

        for (int row = height - 1; row >= 0; row--) {
            double y_val = min_y + (max_y - min_y) * row / (height - 1);
            fprintf(output, "%6.2f |", y_val);

            for (int col = 0; col < width; col++) {
                double x_val = min_x + (max_x - min_x) * col / (width - 1);
                char symbol = ' ';

                // 检查是否有数据点接近这个位置
                for (int i = 0; i < ctx->data_count; i++) {
                    double dx = fabs(ctx->data[i].x - x_val);
                    double dy = fabs(ctx->data[i].y - y_val);
                    double threshold_x = (max_x - min_x) / width;
                    double threshold_y = (max_y - min_y) / height;

                    if (dx <= threshold_x && dy <= threshold_y) {
                        symbol = ctx->type == GRAPH_TYPE_BAR ? '#' :
                                ctx->type == GRAPH_TYPE_LINE ? '*' : '+';
                        break;
                    }
                }

                fprintf(output, "%c", symbol);
            }
            fprintf(output, "\n");
        }

        fprintf(output, "       ");
        for (int i = 0; i < width; i++) fprintf(output, "-");
        fprintf(output, "\n");
        fprintf(output, "       %6.2f", min_x);
        fprintf(output, "%*s", width - 12, "");
        fprintf(output, "%6.2f\n", max_x);
    }
}

/* 生成统计摘要 */
void graph_generate_stats(GraphContext* ctx, FILE* output) {
    if (!ctx || !output || ctx->data_count == 0) return;

    fprintf(output, "\n=== Statistics Summary ===\n");

    // 计算基本统计量
    double sum_x = 0, sum_y = 0;
    double min_x = ctx->data[0].x, max_x = ctx->data[0].x;
    double min_y = ctx->data[0].y, max_y = ctx->data[0].y;

    for (int i = 0; i < ctx->data_count; i++) {
        sum_x += ctx->data[i].x;
        sum_y += ctx->data[i].y;

        if (ctx->data[i].x < min_x) min_x = ctx->data[i].x;
        if (ctx->data[i].x > max_x) max_x = ctx->data[i].x;
        if (ctx->data[i].y < min_y) min_y = ctx->data[i].y;
        if (ctx->data[i].y > max_y) max_y = ctx->data[i].y;
    }

    double mean_x = sum_x / ctx->data_count;
    double mean_y = sum_y / ctx->data_count;

    // 计算方差
    double var_x = 0, var_y = 0;
    for (int i = 0; i < ctx->data_count; i++) {
        var_x += (ctx->data[i].x - mean_x) * (ctx->data[i].x - mean_x);
        var_y += (ctx->data[i].y - mean_y) * (ctx->data[i].y - mean_y);
    }
    var_x /= ctx->data_count;
    var_y /= ctx->data_count;

    double stddev_x = sqrt(var_x);
    double stddev_y = sqrt(var_y);

    fprintf(output, "Data points: %d\n", ctx->data_count);
    fprintf(output, "\nX Statistics:\n");
    fprintf(output, "  Min: %g, Max: %g\n", min_x, max_x);
    fprintf(output, "  Mean: %g, StdDev: %g\n", mean_x, stddev_x);
    fprintf(output, "\nY Statistics:\n");
    fprintf(output, "  Min: %g, Max: %g\n", min_y, max_y);
    fprintf(output, "  Mean: %g, StdDev: %g\n", mean_y, stddev_y);
}

/* 生成图形文件 */
int graph_generate_file(GraphContext* ctx, const char* filename) {
    if (!ctx || !filename) return 0;

    FILE* file = fopen(filename, "w");
    if (!file) return 0;

    // 生成标题
    fprintf(file, "# %s\n", ctx->title ? ctx->title : "Graph");
    fprintf(file, "# Generated by SIMSCRIPT Compiler\n\n");

    // 生成数据
    fprintf(file, "# Data points\n");
    for (int i = 0; i < ctx->data_count; i++) {
        fprintf(file, "%g %g", ctx->data[i].x, ctx->data[i].y);
        if (ctx->data[i].label) {
            fprintf(file, " # %s", ctx->data[i].label);
        }
        fprintf(file, "\n");
    }

    // 生成文本图形
    graph_generate_text(ctx, file);

    // 生成统计信息
    graph_generate_stats(ctx, file);

    fclose(file);
    return 1;
}