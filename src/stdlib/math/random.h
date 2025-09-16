#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Random number generator for SIMSCRIPT */
typedef struct Random {
    uint64_t state;
    uint64_t inc;
} Random;

/* Initialize random number generator with seed */
void random_init(Random* rng, uint64_t seed);

/* Generate uniform random double in [0, 1) */
double random_uniform(Random* rng);

/* Generate uniform random integer in [min, max] */
int random_uniform_int(Random* rng, int min, int max);

/* Generate normal (Gaussian) random number with mean and stddev */
double random_normal(Random* rng, double mean, double stddev);

/* Generate exponential random number with rate parameter */
double random_exponential(Random* rng, double rate);

/* Generate Poisson random number with lambda parameter */
int random_poisson(Random* rng, double lambda);

/* Generate random number from triangular distribution */
double random_triangular(Random* rng, double min, double mode, double max);

/* Generate random number from beta distribution */
double random_beta(Random* rng, double alpha, double beta);

/* Generate random number from gamma distribution */
double random_gamma(Random* rng, double shape, double scale);

/* Generate random number from Weibull distribution */
double random_weibull(Random* rng, double shape, double scale);

/* Generate random number from log-normal distribution */
double random_lognormal(Random* rng, double mean, double stddev);

/* Set seed for global random number generator */
void random_seed(uint64_t seed);

/* Generate uniform random double in [0, 1) using global generator */
double random_uniform_global(void);

/* Generate uniform random integer in [min, max] using global generator */
int random_uniform_int_global(int min, int max);

/* Generate normal random number using global generator */
double random_normal_global(double mean, double stddev);

/* Generate exponential random number using global generator */
double random_exponential_global(double rate);

/* Generate Poisson random number using global generator */
int random_poisson_global(double lambda);

#ifdef __cplusplus
}
#endif