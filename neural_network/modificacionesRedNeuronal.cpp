#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <vector>
#include <immintrin.h>
#include <algorithm>
#include <omp.h>


// Tamaño de las capas de la red
constexpr int INPUT_SIZE = 2565;
constexpr int H1 = 2048;
constexpr int H2 = 1536;
constexpr int H3 = 1024;
constexpr int H4 = 768;
constexpr int H5 = 512;
constexpr int H6 = 256;
constexpr int OUTPUT_SIZE = 1;

// Parámetros de entrenamiento
constexpr float DROPOUT_RATE = 0.1f;
constexpr float NOISE_STDDEV = 0.01f;
constexpr float L2_REGULARIZATION = 0.0001f;
constexpr float LEARNING_RATE = 0.001f;
constexpr float BETA1 = 0.9f;
constexpr float BETA2 = 0.999f;
constexpr float EPSILON = 1e-8f;

std::default_random_engine rng(std::random_device{}());
std::normal_distribution<float> noise_dist(0.0f, NOISE_STDDEV);

inline float relu(float x) { return x > 0.0f ? x : 0.0f; }
inline float drelu(float x) { return x > 0.0f ? 1.0f : 0.0f; }
inline bool dropout_mask() { return ((float)rand() / RAND_MAX) > DROPOUT_RATE; }

// Estructura de Adam para cada parámetro
template<int SIZE>
struct AdamParams {
    float m[SIZE];
    float v[SIZE];
    void init() {
        for (int i = 0; i < SIZE; ++i) { 
            m[i] = 0.0f; v[i] = 0.0f; 
        }
    }
};

// Capa de la red

#pragma pack(push, 32) //evita la fregmentacion interna de la memoria
template<int IN, int OUT>
struct Layer {
    alignas(32) float weights[OUT][IN];
    alignas(32) float biases[OUT];
    alignas(32) float output[OUT];
    alignas(32) float preactivations[OUT];

    AdamParams<OUT> bias_adam;
    AdamParams<OUT * IN> weight_adam;

    void init() {
        for (int i = 0; i < OUT; ++i) {
            biases[i] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            preactivations[i] = 0.0f;
            for (int j = 0; j < IN; ++j) {
                weights[i][j] = ((float)rand() / RAND_MAX - 0.5f) * std::sqrt(2.0f / IN);
            }
        }
        bias_adam.init();
        weight_adam.init();
    }

    void forward(const float* input, bool apply_relu = true, bool apply_dropout = false) {
        #pragma omp parallel for
        for (int i = 0; i < OUT; ++i) {
            __m256 sum_vec = _mm256_setzero_ps();
            int j = 0;
            for (; j <= IN - 8; j += 8) {
                __m256 in = _mm256_loadu_ps(input + j);
                __m256 w = _mm256_loadu_ps(weights[i] + j);
                sum_vec = _mm256_fmadd_ps(in, w, sum_vec);
            }
            float sum = biases[i];
            alignas(32) float temp[8];
            _mm256_store_ps(temp, sum_vec);
            for (int k = 0; k < 8; ++k) sum += temp[k];
            for (; j < IN; ++j) {
                sum += weights[i][j] * input[j];
            }

            float act = apply_relu ? relu(sum) : sum;
            output[i] = (apply_dropout && dropout_mask()) ? 0.0f : act;
            preactivations[i] = sum;
        }
    }

    float regularization_loss() const {
        float reg = 0.0f;
        int in_aligned = (IN / 8) * 8; // Process in chunks of 8
    
        #pragma omp parallel for reduction(+:reg)
        for (int i = 0; i < OUT; ++i) {
            __m256 sum_vec = _mm256_setzero_ps();
            int j = 0;
            for (; j < in_aligned; j += 8) {
                __m256 w_vec = _mm256_loadu_ps(weights[i] + j);
                __m256 w_squared_vec = _mm256_mul_ps(w_vec, w_vec);
                sum_vec = _mm256_add_ps(sum_vec, w_squared_vec);
            }
    
            // Horizontal sum of the vector
            alignas(32) float temp[8];
            _mm256_store_ps(temp, sum_vec);
            float local_reg = 0.0f;
            for (int k = 0; k < 8; ++k) {
                local_reg += temp[k];
            }
    
            // Handle remaining elements (less than 8)
            for (; j < IN; ++j) {
                local_reg += weights[i][j] * weights[i][j];
            }
    
            reg += local_reg;
        }
        return reg;
    }
};
#pragma pack(pop)

struct Network {
    Layer<INPUT_SIZE, H1> l1;
    Layer<H1, H2> l2;
    Layer<H2, H3> l3;
    Layer<H3, H4> l4;
    Layer<H4, H5> l5;
    Layer<H5, H6> l6;
    Layer<H6, OUTPUT_SIZE> out;

    int t;

    void init() {
        l1.init(); l2.init(); l3.init();
        l4.init(); l5.init(); l6.init();
        out.init();
        t = 1;
    }

    float forward(const float* input) {
        l1.forward(input);
        l2.forward(l1.output);
        l3.forward(l2.output);
        l4.forward(l3.output);
        l5.forward(l4.output, true, true);
        l6.forward(l5.output);
        out.forward(l6.output, false);
        return out.output[0];
    }

    float loss(float pred, float target) {
        float mse = 0.5f * (pred - target) * (pred - target);
        float reg = l1.regularization_loss() + l2.regularization_loss() +
                    l3.regularization_loss() + l4.regularization_loss() +
                    l5.regularization_loss() + l6.regularization_loss() +
                    out.regularization_loss();
        return mse + L2_REGULARIZATION * reg;
    }

    void train(float*[] input, float target) {
        float pred = forward(input);
        float loss_value = loss(pred, target);

    }
};

int main() {
    srand(time(NULL));
    Network net;
    net.init();

    std::vector<std::pair<std::vector<float>, float>> training_data;
    // Agregar datos reales aqui

    net.train(training_data, 10, 32);

    return 0;
}
