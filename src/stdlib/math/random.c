#include "random.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

/* PCG32 random number generator constants */
#define PCG32_DEFAULT_STATE  0x853c49e6748fea9bULL
#define PCG32_DEFAULT_STREAM 0xda3e39cb94b95bdbULL
#define PCG32_MULT           0x5851f42d4c957f2dULL

/* Global random number generator */
static Random global_rng;

/* Internal PCG32 implementation */
static uint32_t pcg32_random(Random* rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * PCG32_MULT + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void random_init(Random* rng, uint64_t seed) {
    if (rng == NULL) {
        return;
    }

    rng->state = seed + PCG32_DEFAULT_STREAM;
    rng->inc = (seed << 1) | 1;
    /* Warm up the generator */
    pcg32_random(rng);
}

double random_uniform(Random* rng) {
    if (rng == NULL) {
        return 0.0;
    }
    return (double)pcg32_random(rng) / (double)UINT32_MAX;
}

int random_uniform_int(Random* rng, int min, int max) {
    if (rng == NULL || min > max) {
        return min;
    }
    double r = random_uniform(rng);
    return min + (int)((max - min + 1) * r);
}

double random_normal(Random* rng, double mean, double stddev) {
    if (rng == NULL) {
        return mean;
    }

    /* Box-Muller transform */
    double u1 = random_uniform(rng);
    double u2 = random_uniform(rng);

    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return mean + z0 * stddev;
}

double random_exponential(Random* rng, double rate) {
    if (rng == NULL || rate <= 0.0) {
        return 0.0;
    }

    double u = random_uniform(rng);
    return -log(u) / rate;
}

int random_poisson(Random* rng, double lambda) {
    if (rng == NULL || lambda <= 0.0) {
        return 0;
    }

    if (lambda < 30.0) {
        /* Knuth's algorithm */
        double L = exp(-lambda);
        int k = 0;
        double p = 1.0;

        do {
            k++;
            p *= random_uniform(rng);
        } while (p > L);

        return k - 1;
    } else {
        /* Normal approximation */
        double normal = random_normal(rng, lambda, sqrt(lambda));
        return (int)(normal + 0.5);
    }
}

double random_triangular(Random* rng, double min, double mode, double max) {
    if (rng == NULL || min >= max || mode < min || mode > max) {
        return min;
    }

    double u = random_uniform(rng);
    double F = (mode - min) / (max - min);

    if (u <= F) {
        return min + sqrt(u * (max - min) * (mode - min));
    } else {
        return max - sqrt((1.0 - u) * (max - min) * (max - mode));
    }
}

double random_beta(Random* rng, double alpha, double beta) {
    if (rng == NULL || alpha <= 0.0 || beta <= 0.0) {
        return 0.0;
    }

    /* Using relationship with gamma distribution */
    double x = random_gamma(rng, alpha, 1.0);
    double y = random_gamma(rng, beta, 1.0);
    return x / (x + y);
}

double random_gamma(Random* rng, double shape, double scale) {
    if (rng == NULL || shape <= 0.0 || scale <= 0.0) {
        return 0.0;
    }

    if (shape >= 1.0) {
        /* Ahrens-Dieter algorithm */
        double d = shape - 1.0/3.0;
        double c = 1.0 / sqrt(9.0 * d);

        while (1) {
            double x, v;
            do {
                x = random_normal(rng, 0.0, 1.0);
                v = 1.0 + c * x;
            } while (v <= 0.0);

            v = v * v * v;
            double u = random_uniform(rng);
            double x2 = x * x;

            if (u < 1.0 - 0.0331 * x2 * x2) {
                return scale * d * v;
            }

            if (log(u) < 0.5 * x2 + d * (1.0 - v + log(v))) {
                return scale * d * v;
            }
        }
    } else {
        /* Use beta distribution relationship */
        return scale * random_gamma(rng, shape + 1.0, 1.0) * pow(random_uniform(rng), 1.0/shape);
    }
}

double random_weibull(Random* rng, double shape, double scale) {
    if (rng == NULL || shape <= 0.0 || scale <= 0.0) {
        return 0.0;
    }

    double u = random_uniform(rng);
    return scale * pow(-log(u), 1.0/shape);
}

double random_lognormal(Random* rng, double mean, double stddev) {
    if (rng == NULL) {
        return 1.0;
    }

    double normal = random_normal(rng, 0.0, 1.0);
    return exp(mean + stddev * normal);
}

/* Global random number generator functions */
void random_seed(uint64_t seed) {
    random_init(&global_rng, seed);
}

double random_uniform_global(void) {
    return random_uniform(&global_rng);
}

int random_uniform_int_global(int min, int max) {
    return random_uniform_int(&global_rng, min, max);
}

double random_normal_global(double mean, double stddev) {
    return random_normal(&global_rng, mean, stddev);
}

double random_exponential_global(double rate) {
    return random_exponential(&global_rng, rate);
}

int random_poisson_global(double lambda) {
    return random_poisson(&global_rng, lambda);
}

/* Initialize global generator with current time on first use */
static void init_global_rng(void) {
    static int initialized = 0;
    if (!initialized) {
        random_seed((uint64_t)time(NULL));
        initialized = 1;
    }
}

__attribute__((constructor)) void random_init_global(void) {
    init_global_rng();
}