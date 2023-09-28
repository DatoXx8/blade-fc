#include "nn.h"

long double nn_relu(long double x) {
    return x * (x >= 0);
}
long double nn_d_relu(long double x) {
    return (x >= 0);
}
long double nn_sigmoid(long double x) {
    return 1 / (1 + exp(-x));
}
long double nn_d_sigmoid(long double x) {
    return nn_sigmoid(x) * (1 - nn_sigmoid(x));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ---MATRIX---
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

matrix_t matrix_malloc(uint64_t rows, uint64_t columns) {
    matrix_t m;
    m.rows = rows;
    m.columns = columns;
    m.stride = columns;
    m.values = (long double*)nn_malloc(sizeof(*m.values)*rows*columns);
    assert(m.values != NULL);
    return m;
}
void matrix_dot(matrix_t destination, matrix_t first, matrix_t second) {
    assert(first.columns == second.rows);
    assert(destination.rows == first.rows);
    assert(destination.columns == second.columns);

    uint64_t n = first.columns;

    for (uint64_t i = 0; i < destination.rows; i++) {
        for (uint64_t j = 0; j < destination.columns; j++) {
            MATRIX_AT(destination, i, j) = 0;
            for (uint64_t k = 0; k < n; k++) {
                MATRIX_AT(destination, i, j) += MATRIX_AT(first, i, k) * MATRIX_AT(second, k, j);
            } 
        }
    }
}
matrix_t matrix_row(matrix_t first, uint64_t row) {
    return (matrix_t) {
        .rows = 1,
        .columns = first.columns,
        .stride = first.stride,
        .values = &MATRIX_AT(first, row, 0),
    };
}
void matrix_copy(matrix_t destination, matrix_t first) {
    assert(destination.rows == first.rows);
    //assert(destination.columns == first.columns);

    for (uint64_t i = 0; i < destination.rows; i++) {
        for (uint64_t j = 0; j < destination.columns; j++) {
            MATRIX_AT(destination, i, j) = MATRIX_AT(first, i, j);
        }
    }
}
void matrix_sum(matrix_t destination, matrix_t first) {
    assert(destination.columns == first.columns);
    assert(destination.rows == first.rows);

    for (uint64_t i = 0; i < destination.rows; i++) {
        for (uint64_t j = 0; j < destination.columns; j++) {
            MATRIX_AT(destination, i, j) += MATRIX_AT(first, i, j);
        }
    }
}
void matrix_activate(matrix_t destination) {
    for (uint64_t i = 0; i < destination.rows; i++) {
        for (uint64_t j = 0; j < destination.columns; j++) {
            MATRIX_AT(destination, i, j) = ACTIVATE(MATRIX_AT(destination, i, j));
        }
    }
}
void matrix_print(matrix_t destination, const char *name, uint64_t padding) {
    printf("%*s%s = [\n",(int) padding, "",name);
    for (uint64_t i = 0; i < destination.rows; i++) {
        printf("%*s    ", (int) padding, "");
        for (uint64_t j = 0; j < destination.columns; j++) {
            printf("%Lf ", MATRIX_AT(destination, i, j));
        }
        printf("\n");
    }
    printf("%*s]\n", (int) padding, "");
}
void matrix_fill(matrix_t destination, long double filler) {
    for (uint64_t i = 0; i < destination.rows; i++) {
        for (uint64_t j = 0; j < destination.columns; j++) {
            MATRIX_AT(destination, i, j) = filler;
        }
    }
}
void matrix_random(matrix_t destination, long double bottom, long double top) {
    for (uint64_t i = 0; i < destination.rows; i++) {
        for (uint64_t j = 0; j < destination.columns; j++) {
            MATRIX_AT(destination, i, j) = NN_RAND_DOUBLE(bottom, top);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ---NEURALNET---
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

neuralnet_t neuralnet_malloc(uint64_t *architecture, uint64_t architecture_count) {

    assert(architecture_count > 0);

    neuralnet_t net;
    net.count = architecture_count - 1;

    net.weights = (matrix_t*)malloc(sizeof(*net.weights) * net.count);
    net.biases = (matrix_t*)malloc(sizeof(*net.biases) * net.count);
    net.activations = (matrix_t*)malloc(sizeof(*net.weights) * (net.count + 1));
    assert(net.weights != NULL);
    assert(net.biases != NULL);
    assert(net.activations != NULL);

    net.activations[0] = matrix_malloc(1, architecture[0]);

    for (uint64_t i = 1; i < architecture_count; i++) {
        net.weights[i-1] = matrix_malloc(net.activations[i-1].columns, architecture[i]);
        net.biases[i-1] = matrix_malloc(1, architecture[i]);
        net.activations[i] = matrix_malloc(1, architecture[i]);
    }

    return net;
}
void neuralnet_fill(neuralnet_t net, long double fill) {
    for (uint64_t i = 0; i < net.count; i++) {
        matrix_fill(net.weights[i], fill);
        matrix_fill(net.biases[i], fill);
        matrix_fill(net.activations[i], fill);
    }
    matrix_fill(net.activations[net.count], fill);
}
void neuralnet_random(neuralnet_t net, float bottom, float top) {
    for (uint64_t i = 0; i < net.count; i++) {
        matrix_random(net.weights[i], bottom, top);
        matrix_random(net.biases[i], bottom, top);
    }
}
void neuralnet_nudge(neuralnet_t net, neuralnet_t nudge) {
    neuralnet_random(nudge, -1, 1);
    for (uint64_t i = 0; i < net.count; i++) {
        matrix_sum(net.weights[i], nudge.weights[i]);
        matrix_sum(net.biases[i], nudge.biases[i]);
    }
}
void neuralnet_forward(neuralnet_t net) {
    for (uint64_t i = 0; i < net.count; i++) {
        matrix_dot(net.activations[i+1], net.activations[i], net.weights[i]);
        matrix_sum(net.activations[i+1], net.biases[i]);
        matrix_activate(net.activations[i+1]);
    }
}
long double neuralnet_cost(neuralnet_t net, matrix_t input, matrix_t output) {
    assert(input.rows == output.rows);

    uint64_t num = input.rows;

    long double cost = 0;

    for (uint64_t i = 0; i < num; i++) {
        matrix_t x = matrix_row(input, i);
        matrix_t y = matrix_row(output, i);

        matrix_copy(NN_INPUT(net), x);
        neuralnet_forward(net);
        uint64_t q = output.columns;
        for (uint64_t j = 0; j < q; j++) {
            float difference = MATRIX_AT(NN_OUTPUT(net), 0, j) - MATRIX_AT(y, 0, j);
            cost += difference * difference;
        }
    }

    return cost/num;
}
void neuralnet_backprop(neuralnet_t net, neuralnet_t gradient, matrix_t input, matrix_t output) {
    assert(input.columns == output.columns);

    uint64_t num = input.rows;

    neuralnet_fill(gradient, 0);

    for (uint64_t i = 0; i < num; i++) {
        matrix_copy(NN_INPUT(net), matrix_row(input, i));
        neuralnet_forward(net);

        for (uint64_t j = 0; j <= net.count; j++) {
            matrix_fill(gradient.activations[j], 0);
        }

        for (uint64_t j = 0; j < output.columns; j++) {
            MATRIX_AT(NN_OUTPUT(gradient), 0, j) = 2 * (MATRIX_AT(NN_OUTPUT(net), 0, j) - MATRIX_AT(output, i, j));
        }

        long double s = 1;

        for (uint64_t l = net.count; l > 0; l--) {
            for (uint64_t j = 0; j < net.activations[l].columns; j++) {
                long double a = MATRIX_AT(net.activations[l], 0, j);
                long double d_a = MATRIX_AT(gradient.activations[l], 0, j);
                long double q_a = D_ACTIVATE(a);
                MATRIX_AT(gradient.biases[l - 1], 0, j) += s * d_a * q_a;
                for (uint64_t k = 0; k < net.activations[l - 1].columns; k++) {
                    long double p_a = MATRIX_AT(net.activations[l - 1], 0, k);
                    long double w = MATRIX_AT(net.weights[l - 1],k ,j);
                    MATRIX_AT(gradient.weights[l - 1], k, j) += s * d_a * q_a * p_a;
                    MATRIX_AT(gradient.activations[l-1], 0, k) += s * d_a * q_a * w;
                }
            }
        }
    }

    for (uint64_t i = 0; i < gradient.count; i++) {
        for (uint64_t j = 0; j < gradient.weights[i].rows; j++) {
            for (uint64_t k = 0; k < gradient.weights[i].columns; k++) {
                MATRIX_AT(gradient.weights[i], j, k) /= num;
            }
        }
        for (uint64_t j = 0; j < gradient.biases[i].rows; j++) {
            for (uint64_t k = 0; k < gradient.biases[i].columns; k++) {
                MATRIX_AT(gradient.biases[i], j, k) /= num;
            }
        }
    }
}
void neuralnet_learn(neuralnet_t net, neuralnet_t gradient, long double rate) {
    for (uint64_t i = 0; i < net.count; i++) {
        for (uint64_t j = 0; j < net.weights[i].rows; j++) {
            for (uint64_t k = 0; k < net.weights[i].columns; k++) {
                MATRIX_AT(net.weights[i], j, k) -= rate * MATRIX_AT(gradient.weights[i], j, k);
            }
        }
        for (uint64_t j = 0; j < net.biases[i].rows; j++) {
            for (uint64_t k = 0; k < net.biases[i].columns; k++) {
                MATRIX_AT(net.biases[i], j, k) -= rate * MATRIX_AT(gradient.biases[i], j, k);
            }
        }
    }
}
void neuralnet_print(neuralnet_t net, const char *name) {
    char buf[256];
    printf("%s = [\n", name);
    for (uint64_t i = 0; i < net.count; i++) {
        snprintf(buf, sizeof(buf), "ws%ld", i);
        matrix_print(net.weights[i], buf, 4);
        snprintf(buf, sizeof(buf), "bs%ld", i);
        matrix_print(net.biases[i], buf, 4);
    }
    printf("]\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ---BATCHES---
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

batch batch_malloc(uint64_t batch_size, uint64_t input_size, uint64_t output_size) {
    batch b;
    b.input = matrix_malloc(input_size, batch_size);
    b.output = matrix_malloc(output_size, batch_size);
    return b;
}
void batch_make(batch batch, uint64_t batch_size, uint64_t input_size, uint64_t output_size, matrix_t input, matrix_t output) {
    uint64_t x;
    for (uint64_t i = 0; i < batch_size; i++) {
        x = (rand() % (batch_size + 1));
        for (uint64_t j = 0; j < input_size; j++) {
            MATRIX_AT(batch.input, j, i) = MATRIX_AT(input, j, x);
        }
        for (uint64_t j = 0; j < output_size; j++) {
            MATRIX_AT(batch.output, j, i) = MATRIX_AT(output, j, x);
        }
    }
}
void batch_print(batch b, const char *name) {
    printf("%s = [\n", name);
    matrix_print(b.input, "batch_input", 4);
    matrix_print(b.output, "batch_output", 4);
    printf("]\n");
}
