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
#pragma pack(push, 32) //evita la fragmentacion interna de la memoria
template<int IN, int OUT>
struct Layer {
    alignas(32) float weights[OUT][IN];
    alignas(32) float biases[OUT];
    alignas(32) float output[OUT];
    alignas(32) float preactivations[OUT];
    alignas(32) float gradients[OUT]; // Para almacenar gradientes durante backprop

    AdamParams<OUT> bias_adam;
    AdamParams<OUT * IN> weight_adam;

    void init() {
        for (int i = 0; i < OUT; ++i) {
            biases[i] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
            preactivations[i] = 0.0f;
            gradients[i] = 0.0f;
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

            preactivations[i] = sum;
            float act = apply_relu ? relu(sum) : sum;
            output[i] = (apply_dropout && dropout_mask()) ? 0.0f : act;
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
     
    // Método para actualizar pesos y sesgos usando Adam
    void update_params(int t, const float* input, const float* gradients_next) {
        // Actualización de sesgos
        #pragma omp parallel for
        for (int i = 0; i < OUT; ++i) {
            // Gradiente para el sesgo
            float grad_bias = gradients[i];
            
            // Actualización de los momentos para el sesgo con Adam
            bias_adam.m[i] = BETA1 * bias_adam.m[i] + (1.0f - BETA1) * grad_bias;
            bias_adam.v[i] = BETA2 * bias_adam.v[i] + (1.0f - BETA2) * grad_bias * grad_bias;
            
            // Corrección de bias
            float m_corrected = bias_adam.m[i] / (1.0f - std::pow(BETA1, t));
            float v_corrected = bias_adam.v[i] / (1.0f - std::pow(BETA2, t));
            
            // Actualización del sesgo
            biases[i] -= LEARNING_RATE * m_corrected / (std::sqrt(v_corrected) + EPSILON);
        }
        
        // Actualización de pesos
        #pragma omp parallel for
        for (int i = 0; i < OUT; ++i) {
            for (int j = 0; j < IN; ++j) {
                // Gradiente para el peso
                float grad_weight = gradients[i] * input[j] + 2.0f * L2_REGULARIZATION * weights[i][j];
                
                // Índice lineal para el peso en el vector de Adam
                int idx = i * IN + j;
                
                // Actualización de los momentos para el peso con Adam
                weight_adam.m[idx] = BETA1 * weight_adam.m[idx] + (1.0f - BETA1) * grad_weight;
                weight_adam.v[idx] = BETA2 * weight_adam.v[idx] + (1.0f - BETA2) * grad_weight * grad_weight;
                
                // Corrección de bias
                float m_corrected = weight_adam.m[idx] / (1.0f - std::pow(BETA1, t));
                float v_corrected = weight_adam.v[idx] / (1.0f - std::pow(BETA2, t));
                
                // Actualización del peso
                weights[i][j] -= LEARNING_RATE * m_corrected / (std::sqrt(v_corrected) + EPSILON);
            }
        }
    }

    // Método para la propagación hacia atrás
    void backward(const float* input, const float* grad_output, float* grad_input, bool apply_relu = true) {
        // Inicializar gradientes de entrada si es necesario
        if (grad_input) {
            std::fill(grad_input, grad_input + IN, 0.0f);
        }
        
        #pragma omp parallel for
        for (int i = 0; i < OUT; ++i) {
            // Aplicar la derivada de ReLU si es necesario
            float grad = grad_output[i];
            if (apply_relu) {
                grad *= drelu(preactivations[i]);
            }
            gradients[i] = grad;
            
            // Propagar el gradiente a la capa anterior
            if (grad_input) {
                for (int j = 0; j < IN; ++j) {
                    #pragma omp atomic
                    grad_input[j] += grad * weights[i][j];
                }
            }
        }
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

    alignas(32) float grad_l6[H6];
    alignas(32) float grad_l5[H5];
    alignas(32) float grad_l4[H4];
    alignas(32) float grad_l3[H3];
    alignas(32) float grad_l2[H2];
    alignas(32) float grad_l1[H1];
    alignas(32) float grad_input[INPUT_SIZE];

    int t; // Contador de pasos para Adam

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
        l5.forward(l4.output, true, true); // Aplicamos dropout solo en esta capa
        l6.forward(l5.output);
        out.forward(l6.output, false); // Sin ReLU en la capa de salida
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

    void train(const float* input, float target) {
        // Forward pass
        float pred = forward(input);
        float loss_value = loss(pred, target);
        
        // Calcular el gradiente de salida (derivada de MSE)
        float grad_output = pred - target;
        
        // Backpropagation
        out.gradients[0] = grad_output;
        
        // Propagar hacia atrás a través de todas las capas
        out.backward(l6.output, out.gradients, grad_l6, false);
        l6.backward(l5.output, grad_l6, grad_l5);
        l5.backward(l4.output, grad_l5, grad_l4);
        l4.backward(l3.output, grad_l4, grad_l3);
        l3.backward(l2.output, grad_l3, grad_l2);
        l2.backward(l1.output, grad_l2, grad_l1);
        l1.backward(input, grad_l1, grad_input);
        
        // Actualizar parámetros con Adam
        out.update_params(t, l6.output, nullptr);
        l6.update_params(t, l5.output, grad_l6);
        l5.update_params(t, l4.output, grad_l5);
        l4.update_params(t, l3.output, grad_l4);
        l3.update_params(t, l2.output, grad_l3);
        l2.update_params(t, l1.output, grad_l2);
        l1.update_params(t, input, grad_l1);
        
        // Incrementar el contador de pasos para Adam
        t++;
    }
    
    // Método para evaluar la red en un conjunto de validación
    float evaluate(const std::vector<std::pair<const float*, float>>& validation_data) {
        float total_loss = 0.0f;
        for (const auto& sample : validation_data) {
            float pred = forward(sample.first);
            total_loss += 0.5f * (pred - sample.second) * (pred - sample.second);
        }
        return total_loss / validation_data.size();
    }
};

// Función para entrenar la red
void train_network(Network& network, 
                  const std::vector<std::pair<const float*, float>>& training_data,
                  const std::vector<std::pair<const float*, float>>& validation_data,
                  int epochs, int batch_size) {
    

    
    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Mezclar los datos de entrenamiento
        std::vector<int> indices(training_data.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i] = i;
        }
        std::shuffle(indices.begin(), indices.end(), rng);
        
        // Entrenamiento por lotes
        float epoch_loss = 0.0f;
        for (size_t i = 0; i < training_data.size(); i += batch_size) {
            for (size_t j = i; j < std::min(i + batch_size, training_data.size()); ++j) {
                size_t idx = indices[j];
                const auto& sample = training_data[idx];
                network.train(sample.first, sample.second);
                
                // Añadir ruido gaussiano para regularización
                for (int k = 0; k < INPUT_SIZE; ++k) {
                    network.grad_input[k] += noise_dist(rng);
                }
            }
        }
        
        // Evaluar en el conjunto de validación
        if (!validation_data.empty()) {
            float val_loss = network.evaluate(validation_data);
            std::cout << "Época " << epoch + 1 << "/" << epochs 
                     << ", Pérdida de validación: " << val_loss << std::endl;
        } else {
            std::cout << "Época " << epoch + 1 << "/" << epochs << " completada." << std::endl;
        }
    }
    
    std::cout << "Entrenamiento completado." << std::endl;
}

// Ejemplo de uso
int main() {
    srand(time(nullptr));
    
    // Inicializar la red
    Network network;
    network.init();
    
    // Aquí se deberían cargar y preparar los datos
    std::vector<std::pair<const float*, float>> training_data;
    std::vector<std::pair<const float*, float>> validation_data;
    
    
    /*
    // Cargar datos de entrenamiento
    int num_samples = 1000;
    for (int i = 0; i < num_samples; ++i) {
        float* sample = new float[INPUT_SIZE];
        // Llenar sample con datos...
        float target = /* calcular target basado en sample *//*;
        training_data.push_back({sample, target});
    }
    
    // Cargar datos de validación
    int num_val_samples = 200;
    for (int i = 0; i < num_val_samples; ++i) {
        float* sample = new float[INPUT_SIZE];
        // Llenar sample con datos...
        float target = /* calcular target basado en sample *//*;
        validation_data.push_back({sample, target});
    }
    */
    
    // Entrenar la red
    // train_network(network, training_data, validation_data, 10, 32);
    
  
    
    
    /*
    for (auto& sample : training_data) {
        delete[] sample.first;
    }
    for (auto& sample : validation_data) {
        delete[] sample.first;
    }
    */
    
    return 0;
}
