#include "statistics.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Helper function to compare doubles for sorting */
static int compare_double(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}

double stats_mean(const double* data, int n) {
    if (data == NULL || n <= 0) {
        return 0.0;
    }

    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += data[i];
    }
    return sum / n;
}

double stats_median(double* data, int n) {
    if (data == NULL || n <= 0) {
        return 0.0;
    }

    qsort(data, n, sizeof(double), compare_double);

    if (n % 2 == 0) {
        return (data[n/2 - 1] + data[n/2]) / 2.0;
    } else {
        return data[n/2];
    }
}

double stats_mode(const double* data, int n, int* count) {
    if (data == NULL || n <= 0 || count == NULL) {
        if (count) *count = 0;
        return 0.0;
    }

    double mode = data[0];
    int max_count = 1;
    *count = 1;

    for (int i = 0; i < n; i++) {
        int current_count = 0;
        for (int j = 0; j < n; j++) {
            if (data[j] == data[i]) {
                current_count++;
            }
        }
        if (current_count > max_count) {
            max_count = current_count;
            mode = data[i];
            *count = max_count;
        }
    }

    return mode;
}

double stats_variance(const double* data, int n) {
    if (data == NULL || n <= 1) {
        return 0.0;
    }

    double mean = stats_mean(data, n);
    double sum_sq_diff = 0.0;

    for (int i = 0; i < n; i++) {
        double diff = data[i] - mean;
        sum_sq_diff += diff * diff;
    }

    return sum_sq_diff / (n - 1);
}

double stats_stddev(const double* data, int n) {
    return sqrt(stats_variance(data, n));
}

double stats_skewness(const double* data, int n) {
    if (data == NULL || n <= 2) {
        return 0.0;
    }

    double mean = stats_mean(data, n);
    double stddev = stats_stddev(data, n);
    if (stddev == 0.0) {
        return 0.0;
    }

    double sum_cubed_diff = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = (data[i] - mean) / stddev;
        sum_cubed_diff += diff * diff * diff;
    }

    return sum_cubed_diff / n;
}

double stats_kurtosis(const double* data, int n) {
    if (data == NULL || n <= 3) {
        return 0.0;
    }

    double mean = stats_mean(data, n);
    double stddev = stats_stddev(data, n);
    if (stddev == 0.0) {
        return 0.0;
    }

    double sum_fourth_diff = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = (data[i] - mean) / stddev;
        sum_fourth_diff += diff * diff * diff * diff;
    }

    return (sum_fourth_diff / n) - 3.0;
}

double stats_min(const double* data, int n) {
    if (data == NULL || n <= 0) {
        return 0.0;
    }

    double min_val = data[0];
    for (int i = 1; i < n; i++) {
        if (data[i] < min_val) {
            min_val = data[i];
        }
    }
    return min_val;
}

double stats_max(const double* data, int n) {
    if (data == NULL || n <= 0) {
        return 0.0;
    }

    double max_val = data[0];
    for (int i = 1; i < n; i++) {
        if (data[i] > max_val) {
            max_val = data[i];
        }
    }
    return max_val;
}

double stats_range(const double* data, int n) {
    return stats_max(data, n) - stats_min(data, n);
}

double stats_percentile(const double* data, int n, double percentile) {
    if (data == NULL || n <= 0 || percentile < 0.0 || percentile > 100.0) {
        return 0.0;
    }

    double* sorted_data = (double*)malloc(n * sizeof(double));
    if (sorted_data == NULL) {
        return 0.0;
    }

    memcpy(sorted_data, data, n * sizeof(double));
    qsort(sorted_data, n, sizeof(double), compare_double);

    double index = (percentile / 100.0) * (n - 1);
    int lower = (int)index;
    int upper = lower + 1;

    double result;
    if (upper >= n) {
        result = sorted_data[lower];
    } else {
        double fraction = index - lower;
        result = sorted_data[lower] * (1.0 - fraction) + sorted_data[upper] * fraction;
    }

    free(sorted_data);
    return result;
}

double stats_quartile(const double* data, int n, int quartile) {
    switch (quartile) {
        case 1: return stats_percentile(data, n, 25.0);
        case 2: return stats_median((double*)data, n);
        case 3: return stats_percentile(data, n, 75.0);
        default: return 0.0;
    }
}

double stats_iqr(const double* data, int n) {
    return stats_quartile(data, n, 3) - stats_quartile(data, n, 1);
}

double stats_covariance(const double* x, const double* y, int n) {
    if (x == NULL || y == NULL || n <= 1) {
        return 0.0;
    }

    double mean_x = stats_mean(x, n);
    double mean_y = stats_mean(y, n);
    double sum = 0.0;

    for (int i = 0; i < n; i++) {
        sum += (x[i] - mean_x) * (y[i] - mean_y);
    }

    return sum / (n - 1);
}

double stats_correlation(const double* x, const double* y, int n) {
    double cov = stats_covariance(x, y, n);
    double std_x = stats_stddev(x, n);
    double std_y = stats_stddev(y, n);

    if (std_x == 0.0 || std_y == 0.0) {
        return 0.0;
    }

    return cov / (std_x * std_y);
}

double stats_normal_pdf(double x, double mean, double stddev) {
    if (stddev <= 0.0) {
        return 0.0;
    }

    double diff = x - mean;
    return (1.0 / (stddev * sqrt(2.0 * M_PI))) *
           exp(-0.5 * (diff * diff) / (stddev * stddev));
}

double stats_normal_cdf(double x, double mean, double stddev) {
    if (stddev <= 0.0) {
        return (x >= mean) ? 1.0 : 0.0;
    }

    double z = (x - mean) / stddev;
    return 0.5 * (1.0 + erf(z / sqrt(2.0)));
}

double stats_exponential_pdf(double x, double rate) {
    if (x < 0.0 || rate <= 0.0) {
        return 0.0;
    }
    return rate * exp(-rate * x);
}

double stats_exponential_cdf(double x, double rate) {
    if (x < 0.0 || rate <= 0.0) {
        return 0.0;
    }
    return 1.0 - exp(-rate * x);
}

double stats_poisson_pmf(int k, double lambda) {
    if (k < 0 || lambda <= 0.0) {
        return 0.0;
    }

    double log_pmf = k * log(lambda) - lambda - lgamma(k + 1);
    return exp(log_pmf);
}

TestResult stats_t_test(const double* sample1, int n1,
                       const double* sample2, int n2,
                       double alpha) {
    TestResult result = {0.0, 1.0, false};

    if (sample1 == NULL || sample2 == NULL || n1 <= 1 || n2 <= 1) {
        return result;
    }

    double mean1 = stats_mean(sample1, n1);
    double mean2 = stats_mean(sample2, n2);
    double var1 = stats_variance(sample1, n1);
    double var2 = stats_variance(sample2, n2);

    double se = sqrt(var1/n1 + var2/n2);
    if (se == 0.0) {
        return result;
    }

    result.statistic = (mean1 - mean2) / se;
    /* Simplified p-value calculation (two-tailed) */
    result.p_value = 2.0 * (1.0 - stats_normal_cdf(fabs(result.statistic), 0.0, 1.0));
    result.reject_null = result.p_value < alpha;

    return result;
}

TestResult stats_z_test(double sample_mean, double sample_stddev, int n,
                       double population_mean, double alpha) {
    TestResult result = {0.0, 1.0, false};

    if (sample_stddev <= 0.0 || n <= 0) {
        return result;
    }

    double se = sample_stddev / sqrt(n);
    result.statistic = (sample_mean - population_mean) / se;
    result.p_value = 2.0 * (1.0 - stats_normal_cdf(fabs(result.statistic), 0.0, 1.0));
    result.reject_null = result.p_value < alpha;

    return result;
}

TestResult stats_chi_square_test(const double* observed,
                                const double* expected, int n,
                                double alpha) {
    TestResult result = {0.0, 1.0, false};

    if (observed == NULL || expected == NULL || n <= 0) {
        return result;
    }

    double chi_square = 0.0;
    for (int i = 0; i < n; i++) {
        if (expected[i] > 0.0) {
            double diff = observed[i] - expected[i];
            chi_square += (diff * diff) / expected[i];
        }
    }

    result.statistic = chi_square;
    /* Simplified p-value (using normal approximation for large n) */
    result.p_value = 1.0 - stats_normal_cdf(chi_square, n-1, sqrt(2*(n-1)));
    result.reject_null = result.p_value < alpha;

    return result;
}

ConfidenceInterval stats_mean_ci(const double* data, int n, double confidence) {
    ConfidenceInterval ci = {0.0, 0.0, confidence};

    if (data == NULL || n <= 1 || confidence <= 0.0 || confidence >= 1.0) {
        return ci;
    }

    double mean = stats_mean(data, n);
    double stddev = stats_stddev(data, n);
    double se = stddev / sqrt(n);

    double z = -stats_normal_cdf(0.0, (1.0 - confidence)/2.0, 1.0);
    ci.lower = mean - z * se;
    ci.upper = mean + z * se;

    return ci;
}

ConfidenceInterval stats_proportion_ci(int successes, int trials, double confidence) {
    ConfidenceInterval ci = {0.0, 0.0, confidence};

    if (trials <= 0 || successes < 0 || successes > trials ||
        confidence <= 0.0 || confidence >= 1.0) {
        return ci;
    }

    double p = (double)successes / trials;
    double se = sqrt(p * (1.0 - p) / trials);

    double z = -stats_normal_cdf(0.0, (1.0 - confidence)/2.0, 1.0);
    ci.lower = p - z * se;
    ci.upper = p + z * se;

    return ci;
}

LinearRegression stats_linear_regression(const double* x, const double* y, int n) {
    LinearRegression lr = {0.0, 0.0, 0.0, 0.0};

    if (x == NULL || y == NULL || n <= 1) {
        return lr;
    }

    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0, sum_y2 = 0.0;

    for (int i = 0; i < n; i++) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xy += x[i] * y[i];
        sum_x2 += x[i] * x[i];
        sum_y2 += y[i] * y[i];
    }

    double denominator = n * sum_x2 - sum_x * sum_x;
    if (denominator == 0.0) {
        return lr;
    }

    lr.slope = (n * sum_xy - sum_x * sum_y) / denominator;
    lr.intercept = (sum_y * sum_x2 - sum_x * sum_xy) / denominator;
    lr.correlation = stats_correlation(x, y, n);
    lr.r_squared = lr.correlation * lr.correlation;

    return lr;
}

MovingStats* moving_stats_create(int window_size) {
    if (window_size <= 0) {
        return NULL;
    }

    MovingStats* ms = (MovingStats*)malloc(sizeof(MovingStats));
    if (ms == NULL) {
        return NULL;
    }

    ms->window = (double*)malloc(window_size * sizeof(double));
    if (ms->window == NULL) {
        free(ms);
        return NULL;
    }

    ms->size = window_size;
    ms->count = 0;
    ms->index = 0;
    ms->sum = 0.0;
    ms->sum_sq = 0.0;

    return ms;
}

void moving_stats_destroy(MovingStats* ms) {
    if (ms != NULL) {
        free(ms->window);
        free(ms);
    }
}

void moving_stats_add(MovingStats* ms, double value) {
    if (ms == NULL) {
        return;
    }

    if (ms->count < ms->size) {
        ms->window[ms->count] = value;
        ms->sum += value;
        ms->sum_sq += value * value;
        ms->count++;
    } else {
        double old_value = ms->window[ms->index];
        ms->sum -= old_value;
        ms->sum_sq -= old_value * old_value;

        ms->window[ms->index] = value;
        ms->sum += value;
        ms->sum_sq += value * value;

        ms->index = (ms->index + 1) % ms->size;
    }
}

double moving_stats_mean(MovingStats* ms) {
    if (ms == NULL || ms->count == 0) {
        return 0.0;
    }
    return ms->sum / ms->count;
}

double moving_stats_variance(MovingStats* ms) {
    if (ms == NULL || ms->count <= 1) {
        return 0.0;
    }

    double mean = moving_stats_mean(ms);
    double variance = (ms->sum_sq / ms->count) - (mean * mean);
    return variance * ms->count / (ms->count - 1);
}

double moving_stats_stddev(MovingStats* ms) {
    return sqrt(moving_stats_variance(ms));
}