#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Statistics functions for SIMSCRIPT */

/* Basic statistical measures */
double stats_mean(const double* data, int n);
double stats_median(double* data, int n);
double stats_mode(const double* data, int n, int* count);
double stats_variance(const double* data, int n);
double stats_stddev(const double* data, int n);
double stats_skewness(const double* data, int n);
double stats_kurtosis(const double* data, int n);
double stats_min(const double* data, int n);
double stats_max(const double* data, int n);
double stats_range(const double* data, int n);

/* Percentiles and quantiles */
double stats_percentile(const double* data, int n, double percentile);
double stats_quartile(const double* data, int n, int quartile);
double stats_iqr(const double* data, int n);

/* Correlation and covariance */
double stats_covariance(const double* x, const double* y, int n);
double stats_correlation(const double* x, const double* y, int n);

/* Distribution tests */
double stats_normal_pdf(double x, double mean, double stddev);
double stats_normal_cdf(double x, double mean, double stddev);
double stats_exponential_pdf(double x, double rate);
double stats_exponential_cdf(double x, double rate);
double stats_poisson_pmf(int k, double lambda);

/* Hypothesis testing */
typedef struct {
    double statistic;
    double p_value;
    bool reject_null;
} TestResult;

TestResult stats_t_test(const double* sample1, int n1,
                       const double* sample2, int n2,
                       double alpha);
TestResult stats_z_test(double sample_mean, double sample_stddev, int n,
                       double population_mean, double alpha);
TestResult stats_chi_square_test(const double* observed,
                                const double* expected, int n,
                                double alpha);

/* Confidence intervals */
typedef struct {
    double lower;
    double upper;
    double confidence_level;
} ConfidenceInterval;

ConfidenceInterval stats_mean_ci(const double* data, int n, double confidence);
ConfidenceInterval stats_proportion_ci(int successes, int trials, double confidence);

/* Regression analysis */
typedef struct {
    double slope;
    double intercept;
    double r_squared;
    double correlation;
} LinearRegression;

LinearRegression stats_linear_regression(const double* x, const double* y, int n);

/* Moving statistics */
typedef struct {
    double* window;
    int size;
    int count;
    int index;
    double sum;
    double sum_sq;
} MovingStats;

MovingStats* moving_stats_create(int window_size);
void moving_stats_destroy(MovingStats* ms);
void moving_stats_add(MovingStats* ms, double value);
double moving_stats_mean(MovingStats* ms);
double moving_stats_variance(MovingStats* ms);
double moving_stats_stddev(MovingStats* ms);

#ifdef __cplusplus
}
#endif