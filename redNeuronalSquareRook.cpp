#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>

//tamaño de las capas de la red neuronal
constexpr int INPUT_SIZE = 2565;
constexpr int H1 = 2048;
constexpr int H2 = 1536;
constexpr int H3 = 1024;
constexpr int H4 = 768;
constexpr int H5 = 512;
constexpr int H6 = 256;
constexpr int OUTPUT_SIZE = 1;

constexpr float DROPOUT_RATE = 0.1f;
constexpr float NOISE_STDDEV = 0.01f;
constexpr float L2_REGULARIZATION = 0.0001f;

//"motor" de numeros pseudoaleatorios
std::default_random_engine rng(std::random_device{}());
//Distribucion normal para el ruido
std::normal_distribution<float> noise_dist(0.0f, NOISE_STDDEV);

//inline basicamente lo que hace es decirle al compilador que no haga un llamado a la funcion, 
//sino que la reemplace por el código de la función en sí
// fucnion RELU
inline float relu(float x) {
    return x > 0 ? x : 0;
}

// Dropout mask
inline bool dropout_mask() {
    return ((float)rand() / RAND_MAX) > DROPOUT_RATE;
}

// Estructura general de una capa totalmente conectada
template<int IN, int OUT>
struct Layer {
    float weights[OUT][IN];
    float biases[OUT];
    float output[OUT];
    //pesos al azar entre -0.5 y 0.5
    //biases al azar entre -0.5 y 0.5
    void init() {
        for (int i = 0; i < OUT; ++i) {
            biases[i] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            for (int j = 0; j < IN; ++j) {
                weights[i][j] = ((float)rand() / RAND_MAX - 0.5f) * sqrtf(2.0f / IN);  
            }
        }
    }

    void forward(const float* input, bool apply_relu = true, bool apply_dropout = false) {
        for (int i = 0; i < OUT; ++i) {
            float sum = biases[i];
            for (int j = 0; j < IN; ++j) {
                float noisy_input = input[j] + noise_dist(rng);
                sum += weights[i][j] * noisy_input;
            }
            float act = apply_relu ? relu(sum) : sum;
            output[i] = (apply_dropout && dropout_mask()) ? 0.0f : act;
        }
    }

    float regularization_loss() const {
        float reg = 0.0f;
        for (int i = 0; i < OUT; ++i)
            for (int j = 0; j < IN; ++j)
                reg += weights[i][j] * weights[i][j];
        return reg;
    }
};

struct Network {
    Layer<INPUT_SIZE, H1> l1;
    Layer<H1, H2> l2;
    Layer<H2, H3> l3;
    Layer<H3, H4> l4;
    Layer<H4, H5> l5;
    Layer<H5, H6> l6;
    Layer<H6, OUTPUT_SIZE> out;

    void init() {
        l1.init(); l2.init(); l3.init();
        l4.init(); l5.init(); l6.init();
        out.init();
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
        float reg = (
            l1.regularization_loss() + l2.regularization_loss() + l3.regularization_loss() +
            l4.regularization_loss() + l5.regularization_loss() + l6.regularization_loss() +
            out.regularization_loss());
        return mse + L2_REGULARIZATION * reg;
    }
};
