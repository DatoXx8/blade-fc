#ifndef CHESS_H_

#define CHESS_H_

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "stdbool.h"
#include "math.h"
#include "stdint.h"

#define NN_ARRAY_LEN(x) sizeof((x))/sizeof((x)[0])
#define NN_RAND_DOUBLE(bottom, top) (long double)rand()/RAND_MAX * ((top) - (bottom)) + (bottom)

#ifndef nn_malloc
#define nn_malloc malloc
#endif

extern long double nn_relu(long double x);
extern long double nn_d_relu(long double x);
extern long double nn_sigmoid(long double x);
extern long double nn_d_sigmoid(long double x);

#define ACTIVATE(x) nn_relu((x))
#define D_ACTIVATE(x) nn_d_relu((x))

typedef struct {
    uint64_t rows;
    uint64_t columns;
    uint64_t stride;
    long double *values;
} matrix_t;

#define MATRIX_AT(m, i, j) (m).values[(i) * (m).stride + (j)]

typedef struct {
    uint64_t count; // amount of weights and biases
    matrix_t *weights;
    matrix_t *biases;
    matrix_t *activations;
} neuralnet_t;

#define NN_INPUT(n) (n).activations[0]
#define NN_OUTPUT(n) (n).activations[(n).count]

typedef struct {
    matrix_t input;
    matrix_t output;
} batch;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ---MATRIX---
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern matrix_t matrix_malloc(uint64_t rows, uint64_t columns);
extern void matrix_dot(matrix_t destination, matrix_t first, matrix_t second);
extern matrix_t matrix_row(matrix_t first, uint64_t row);
extern void matrix_copy(matrix_t destination, matrix_t first);
extern void matrix_sum(matrix_t destination, matrix_t first);
extern void matrix_activate(matrix_t destination);
extern void matrix_print(matrix_t destination, const char *name, uint64_t padding);
extern void matrix_fill(matrix_t destination, long double filler);
extern void matrix_random(matrix_t destination, long double bottom, long double top);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ---NEURALNET---
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern neuralnet_t neuralnet_malloc(uint64_t *architecture, uint64_t architecture_count);
extern void neuralnet_fill(neuralnet_t net, long double fill);
extern void neuralnet_random(neuralnet_t net, float bottom, float top);
extern void neuralnet_nudge(neuralnet_t net, neuralnet_t nudge);
extern void neuralnet_forward(neuralnet_t net);
extern long double neuralnet_cost(neuralnet_t net, matrix_t input, matrix_t output);
extern void neuralnet_backprop(neuralnet_t net, neuralnet_t gradient, matrix_t input, matrix_t output);
extern void neuralnet_learn(neuralnet_t net, neuralnet_t gradient, long double rate);
extern void neuralnet_print(neuralnet_t net, const char *name);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ---BATCHES---
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern batch batch_malloc(uint64_t batch_size, uint64_t input_size, uint64_t output_size);
extern void batch_make(batch batch, uint64_t batch_size, uint64_t input_size, uint64_t output_size, matrix_t input, matrix_t output);
extern void batch_print(batch b, const char *name);

#endif
