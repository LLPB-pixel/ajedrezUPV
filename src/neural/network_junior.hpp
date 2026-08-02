#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <algorithm>
#include <omp.h>
#include <fstream>
#include "json.hpp"
#include <future>
#include <string>
#include <chrono>

using json = nlohmann::json;
// Tamaño de las capas de la red
constexpr int INPUT_SIZE = 773;
constexpr int H1 = 512;
constexpr int H2 = 512;
constexpr int OUTPUT_SIZE = 1;

// Parámetros de entrenamiento
constexpr float DROPOUT_RATE = 0.1f;
constexpr float NOISE_STDDEV = 0.01f;
constexpr float L2_REGULARIZATION = 0.0005f;
constexpr float LEARNING_RATE = 0.0001f;
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
        #pragma omp parallel for
        for (int i = 0; i < SIZE; ++i) { 
            m[i] = 0.01f; v[i] = 0.01f; 
        }
    }
};

// Capa de la red
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
        #pragma omp parallel for
        for (int i = 0; i < OUT; ++i) {
            biases[i] = (((float)rand() / RAND_MAX - 0.5f) * 0.1f)/(10000.0f);
            preactivations[i] = 0.0f;
            gradients[i] = 0.0f;
            for (int j = 0; j < IN; ++j) {
                weights[i][j] = (((float)rand() / RAND_MAX - 0.5f) * std::sqrt(2.0f / IN))/(10000.0f);
            }
        }
        bias_adam.init();
        weight_adam.init();
    }

    void forward(const float* input, bool apply_relu = true, bool apply_dropout = false) {
        #pragma omp parallel for
        for (int i = 0; i < OUT; ++i) {
            float sum = biases[i];
            for (int j = 0; j < IN; ++j) {
                sum += weights[i][j] * input[j];
            }
            preactivations[i] = sum;
            float act = apply_relu ? relu(sum) : sum;
            output[i] = (apply_dropout && dropout_mask()) ? 0.0f : act;
        }
    }

    float regularization_loss() const {
        float reg = 0.0f;
        #pragma omp parallel for reduction(+:reg)
        for (int i = 0; i < OUT; ++i) {
            float local_reg = 0.0f;
            for (int j = 0; j < IN; ++j) {
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

struct Network {
    Layer<INPUT_SIZE, H1> l1;
    Layer<H1, H2> l2;
    Layer<H2, OUTPUT_SIZE> out;

    alignas(32) float grad_l2[H2];
    alignas(32) float grad_l1[H1];
    alignas(32) float grad_input[INPUT_SIZE];

    int t; // Contador de pasos para Adam


    float forward(const float* input) {
        l1.forward(input);
        l2.forward(l1.output, true, true); // Aplicamos dropout en la segunda capa
        out.forward(l2.output, false); // Sin ReLU en la capa de salida
        return out.output[0];
    }

    float loss(float pred, float target) {
        // y la regularización L2 proviene de la suma de los pesos al cuadrado
        float error = pred - target;
        
        // Parámetro delta para Huber Loss, puedes ajustarlo según el rango de tus errores
        const float delta = 1.0f;  // Ajusta este valor según lo que consideres adecuado

        // Huber loss: Si el error es pequeño, usa MSE; si es grande, usa MAE
        float huber_loss = (std::abs(error) <= delta) ? 0.5f * error * error : delta * (std::abs(error) - 0.5f * delta);
        if(this->t > 5000){
            return huber_loss;
        }
        auto f1 = std::async(std::launch::async, [&] { return l1.regularization_loss(); });
        auto f2 = std::async(std::launch::async, [&] { return l2.regularization_loss(); });
        auto f3 = std::async(std::launch::async, [&] { return out.regularization_loss(); });

        float reg = f1.get() + f2.get() + f3.get();
        
        return huber_loss + L2_REGULARIZATION * reg;
    }

    // Método para entrenar la red con una única muestra
    void train(const float* input, float target) {
        // Forward pass
        float pred = forward(input);
        // Calcular el gradiente de salida (derivada de MSE)
        float grad_output = pred - target;
        
        // Backpropagation
        out.gradients[0] = grad_output;
        
        // Propagar hacia atrás a través de todas las capas
        out.backward(l2.output, out.gradients, grad_l2, false);
        l2.backward(l1.output, grad_l2, grad_l1);
        l1.backward(input, grad_l1, grad_input);
        
        // Actualizar parámetros con Adam
        out.update_params(t, l2.output, nullptr);
        l2.update_params(t, l1.output, grad_l2);
        l1.update_params(t, input, grad_l1);
        
        // Incrementar el contador de pasos para Adam
        t++;
    }
    template<int IN_SIZE, int OUT_SIZE>
        void serialize_layer(Layer<IN_SIZE, OUT_SIZE>& layer, const std::string& layer_name, json& output_json) {
            json layer_json;
            json weights_json = json::array();
            json biases_json = json::array();
            
            // Crear arrays paralelos para pesos
            #pragma omp parallel
            {
                json thread_weights = json::array();
                
                #pragma omp for nowait
                for (int i = 0; i < OUT_SIZE; ++i) {
                    json row = json::array();
                    for (int j = 0; j < IN_SIZE; ++j) {
                        row.push_back(layer.weights[i][j]);
                    }
                    
                    #pragma omp critical
                    {
                        weights_json.push_back(row);
                    }
                }
            }
            
            // Crear array para sesgos
            #pragma omp parallel
            {
                json thread_biases = json::array();
                
                #pragma omp for nowait
                for (int i = 0; i < OUT_SIZE; ++i) {
                    #pragma omp critical
                    {
                        biases_json.push_back(layer.biases[i]);
                    }
                }
            }
            
            layer_json["weights"] = weights_json;
            layer_json["biases"] = biases_json;
            output_json[layer_name] = layer_json;
        };
    void save_to_json(const std::string& filename) {
        json network_json;
        
        std::cout << "Guardando modelo en " << filename << "..." << std::endl;
        
        // Usar std::async para paralelizar la serialización de las capas
        auto f1 = std::async(std::launch::async, [&]{ serialize_layer<INPUT_SIZE, H1>(l1, "layer1", network_json); });
        auto f2 = std::async(std::launch::async, [&]{ serialize_layer<H1, H2>(l2, "layer2", network_json); });
        auto f3 = std::async(std::launch::async, [&]{ serialize_layer<H2, OUTPUT_SIZE>(out, "output", network_json); });
        
        // Esperar a que todas las tareas asíncronas se completen
        f1.wait(); f2.wait(); f3.wait();
        
        // Añadir el contador de pasos de Adam
        network_json["adam_t"] = t;
        
        // Escribir en el archivo
        std::ofstream file(filename);
        if (file.is_open()) {
            file << std::setw(4) << network_json << std::endl;
            std::cout << "Modelo guardado exitosamente." << std::endl;
        } else {
            std::cerr << "Error: No se pudo abrir el archivo para escritura." << std::endl;
        }
    }
    template<int IN_SIZE, int OUT_SIZE>
    void deserialize_layer(Layer<IN_SIZE, OUT_SIZE>& layer, const json& layer_json) {
        const json& weights_json = layer_json["weights"];
        const json& biases_json = layer_json["biases"];
                
                // Cargar pesos
        #pragma omp parallel for
        for (int i = 0; i < OUT_SIZE; ++i) {
            const json& row = weights_json[i];
            for (int j = 0; j < IN_SIZE; ++j) {
                layer.weights[i][j] = row[j];
                }
            }
                
                // Cargar sesgos
            #pragma omp parallel for
            for (int i = 0; i < OUT_SIZE; ++i) {
                layer.biases[i] = biases_json[i];
            }
                
                // Inicializar parámetros de Adam
            layer.bias_adam.init();
            layer.weight_adam.init();
    };

    // Inicializar la red desde un archivo JSON
    bool init_from_json(const std::string& filename) {
        std::cout << "Cargando modelo desde " << filename << "..." << std::endl;
        
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: No se pudo abrir el archivo." << std::endl;
                return false;
            }
            
            json network_json;
            file >> network_json;
            
            
            
            // Usar std::async para paralelizar la carga de las capas
            auto f1 = std::async(std::launch::async, [&]{ 
                if (network_json.contains("layer1")) deserialize_layer<INPUT_SIZE, H1>(l1, network_json["layer1"]); 
            });
            auto f2 = std::async(std::launch::async, [&]{ 
                if (network_json.contains("layer2")) deserialize_layer<H1, H2>(l2, network_json["layer2"]); 
            });
            auto f3 = std::async(std::launch::async, [&]{ 
                if (network_json.contains("output")) deserialize_layer<H2, OUTPUT_SIZE>(out, network_json["output"]); 
            });
            
            // Esperar a que todas las tareas asíncronas se completen
            f1.wait(); f2.wait(); f3.wait();
            
            // Cargar el contador de pasos de Adam
            if (network_json.contains("adam_t")) {
                t = network_json["adam_t"];
            } else {
                t = 1; // Valor por defecto
            }
            
            std::cout << "Modelo cargado exitosamente." << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error al cargar el modelo: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Modificar el método init para incluir la opción de cargar desde JSON
    void init(const std::string& json_file = "") {
        if (!json_file.empty()) {
            if (init_from_json(json_file)) {
                return; // La red ya está inicializada desde el archivo JSON
            }
            std::cout << "Fallback a inicialización aleatoria..." << std::endl;
        }
        
        // Inicialización estándar aleatoria
        l1.init(); l2.init();
        out.init();
        t = 1;
    }
    

};

// Función para entrenar la red
// Función para entrenar la red con múltiples épocas sobre un único ejemplo
void train_network(Network* network, const float* input, float target, int epochs) {
    std::cout << "Iniciando entrenamiento..." << std::endl;
    
    float initial_loss = network->loss(network->forward(input), target);
    std::cout << "Pérdida inicial: " << initial_loss << std::endl;
    
    #pragma omp parallel for
    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Entrenar con la única muestra disponible
        network->train(input, target);
        
        // Cada cierto número de épocas, mostrar el progreso
        if ((epoch + 1) % 100 == 0 || epoch == 0) {
            float current_loss = network->loss(network->forward(input), target);
            std::cout << "Época " << epoch + 1 << "/" << epochs 
                     << ", Pérdida actual: " << current_loss << std::endl;
        }
    }
    
    float final_loss = network->loss(network->forward(input), target);
    std::cout << "Entrenamiento completado. Pérdida final: " << final_loss << std::endl;
}
/*
int main() {
    std::cout << "Entrando a main()" << std::endl;

    Network* network = new Network();  // en el heap
    network->init();

    float* input = new float[INPUT_SIZE];  // en el heap

    #pragma omp parallel for
    for (int i = 0; i < INPUT_SIZE; ++i) {
        input[i] = static_cast<float>(rand());
    }
    std::cout << "Input inicializado" << std::endl;
    
    float target = 0.5f;
    auto start_time = std::chrono::high_resolution_clock::now();
    train_network(network, input, target, 500);  // pasa un puntero
    auto end_time = std::chrono::high_resolution_clock::now();
    
    network->forward(input);
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "Tiempo de entrenamiento: " << duration.count() / 1e6 << " segundos" << std::endl;
    

    delete[] input;
    delete network;
    return 0;
}*/
//g++ -std=c++17 -O3 -g -fopenmp -march=native -o modificacionesRedNeuronal modificacionesRedNeuronal.cpp -lstdc++fs -pthread