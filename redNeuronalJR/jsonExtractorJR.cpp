#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <ctime>
#include <iomanip> 
#include <cassert>
#include "json.hpp"
#include "chess.hpp" 
#include "redNeuronalJR.cpp"
#include <csignal>

using json = nlohmann::json;



struct Registro {
    std::string fen; // Estado del tablero en formato FEN
    int cp;          // Evaluación en centipawns
};

// Variable global para indicar si se debe interrumpir el programa
volatile std::sig_atomic_t interrupcionSolicitada = 0;

// Manejador de señales para SIGINT
void manejadorInterrupcion(int signal) {
    interrupcionSolicitada = 1;
}

void procesarLinea(const std::string& linea, std::vector<Registro>& resultados) {
    json j = json::parse(linea);

    if (!j.contains("fen") || !j["evals"].is_array()) {
        return;
    }

    std::string fen = j["fen"];
    const auto& eval = j["evals"][0];

    if (!eval.contains("pvs") || !eval["pvs"].is_array()) {
        return;
    }

    for (const auto& pv : eval["pvs"]) {
        if (!pv.contains("cp")) {
            continue;
        }

        Registro r;
        r.fen = fen;
        r.cp = pv["cp"];
        resultados.push_back(std::move(r));
    }
}

void guardarProgreso(int lineaActual, const std::string& archivoActual, int registrosProcesados, float perdidaMedia) {
    std::ofstream archivoProgreso("progreso.txt", std::ios::app); // Abrir en modo append
    if (!archivoProgreso.is_open()) {
        std::cerr << "Error al abrir el archivo de progreso.\n";
        return;
    }

    // Obtener la hora actual
    std::time_t ahora = std::time(nullptr);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&ahora));

    // Guardar la línea actual, el archivo actual, la hora, registros procesados y pérdida media
    archivoProgreso << "Archivo: " << archivoActual << " - Línea actual: " << lineaActual 
                   << " - Registros procesados: " << registrosProcesados
                   << " - Pérdida media: " << perdidaMedia
                   << " - Hora: " << buffer << "\n";
    archivoProgreso.close();
}
inline float sigmoidal(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

inline float normalizarCP(int cp) {
    float valor = static_cast<float>(cp);
    valor = valor / 100.0f; // Normalizar a un rango entre -1 y 1
    return sigmoidal(valor); 

}


void convertirFENaInput(const std::string& fen, float* input) {
    // Inicializar la matriz de entrada con ceros
    std::memset(input, 0, sizeof(float) * 768);

    // Crear un objeto Board a partir del FEN
    chess::Board board(fen);

    // Definir los 12 bitboards para las piezas
    chess::Bitboard bitboards[12] = {
        board.pieces(chess::PieceType::PAWN, chess::Color::WHITE),
        board.pieces(chess::PieceType::PAWN, chess::Color::BLACK),
        board.pieces(chess::PieceType::KNIGHT, chess::Color::WHITE),
        board.pieces(chess::PieceType::KNIGHT, chess::Color::BLACK),
        board.pieces(chess::PieceType::BISHOP, chess::Color::WHITE),
        board.pieces(chess::PieceType::BISHOP, chess::Color::BLACK),
        board.pieces(chess::PieceType::ROOK, chess::Color::WHITE),
        board.pieces(chess::PieceType::ROOK, chess::Color::BLACK),
        board.pieces(chess::PieceType::QUEEN, chess::Color::WHITE),
        board.pieces(chess::PieceType::QUEEN, chess::Color::BLACK),
        board.pieces(chess::PieceType::KING, chess::Color::WHITE),
        board.pieces(chess::PieceType::KING, chess::Color::BLACK)
    };

    // Convertir los bitboards en la matriz de entrada
    int index = 0;
    for (int i = 0; i < 12; ++i) {
        for (int bit = 0; bit < 64; ++bit) {
            input[index++] = bitboards[i].check(bit) ? 1.0f : 0.0f;
        }
    }

    // Handle castling rights
    auto castlingRights = board.castlingRights();
    input[index++] = castlingRights.has(chess::Color::WHITE, chess::Board::CastlingRights::Side::KING_SIDE) ? 1.0f : 0.0f;
    input[index++] = castlingRights.has(chess::Color::WHITE, chess::Board::CastlingRights::Side::QUEEN_SIDE) ? 1.0f : 0.0f;
    input[index++] = castlingRights.has(chess::Color::BLACK, chess::Board::CastlingRights::Side::KING_SIDE) ? 1.0f : 0.0f;
    input[index++] = castlingRights.has(chess::Color::BLACK, chess::Board::CastlingRights::Side::QUEEN_SIDE) ? 1.0f : 0.0f;
}

int main() {
    // Registrar el manejador de señales
    std::signal(SIGINT, manejadorInterrupcion);
    
    // Inicializar la red neuronal
    Network* red = new Network();
    
    // Intentar cargar la red desde un archivo JSON si existe
    std::ifstream testArchivo("redNeuronal.json");
    if (testArchivo.good()) {
        testArchivo.close();
        std::cout << "Cargando red neuronal desde archivo existente...\n";
        red->init("redNeuronal.json");
    } else {
        std::cout << "Inicializando nueva red neuronal...\n";
        red->init();
        std::string fen = "rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKB1R w KQkq - 0 1";
        float input[773];
        convertirFENaInput(fen, input);
        float resultado = red->forward(input);
        std::cout << "Resultado inicial: " << resultado << std::endl;
    }
    
    std::vector<Registro> registros;
    float* input = new float[773]; // 768 + 5 para los derechos de enroque y el turno
    
    // Estadísticas de entrenamiento
    int totalRegistrosProcesados = 0;
    float perdidaAcumulada = 0.0f;
    float sumaCuadrados = 0.0f; // Para calcular la desviación típica
    int loteActual = 0;
    const int tamanoLote = 100; // Cada cuántos registros guardar estadísticas

    std::ostringstream nombreArchivo;
    nombreArchivo << "chunk_002.jsonl";

    std::ifstream archivo(nombreArchivo.str());
    if (!archivo.is_open()) {
        std::cerr << "Error al abrir el archivo: " << nombreArchivo.str() << "\n";
        delete[] input;
        delete red;
        return 1; // Salir con error
    }

    std::string linea;
    int contador = 0;

    while (std::getline(archivo, linea)) {
        if (interrupcionSolicitada) {
            std::cerr << "Interrupción detectada. Guardando progreso...\n";
            red->save_to_json("redNeuronal.json");
            guardarProgreso(contador, nombreArchivo.str(), totalRegistrosProcesados, 
                          totalRegistrosProcesados > 0 ? perdidaAcumulada / totalRegistrosProcesados : 0.0f);
            break;
        }

        contador++;
        if (!linea.empty()) {
            try {
                // Limpiar vector de registros para la nueva línea
                registros.clear();
                procesarLinea(linea, registros);
                
                // Procesar cada registro para entrenamiento
                for (auto& registro : registros) {
                    chess::Board board(registro.fen); // Crear el tablero inicial a partir del FEN

                    // Convertir el tablero actual a input
                    convertirFENaInput(registro.fen, input);

                    // Normalizar el valor de centipawns como objetivo
                    float target = normalizarCP(registro.cp);

                    // Calcular pérdida antes del entrenamiento para estadísticas
                    float prediccion = red->forward(input);
                    float perdida = red->loss(prediccion, target);

                    // Entrenar la red con este ejemplo
                    red->train(input, target);

                    // Actualizar estadísticas
                    totalRegistrosProcesados++;
                    perdidaAcumulada += perdida;
                    sumaCuadrados += perdida * perdida;
                    loteActual++;

                    // Mostrar estadísticas cada cierto número de ejemplos
                    if (loteActual >= tamanoLote) {
                        float perdidaMedia = perdidaAcumulada / totalRegistrosProcesados;
                        float varianza = (sumaCuadrados / totalRegistrosProcesados) - (perdidaMedia * perdidaMedia);
                        float desviacionTipica = std::sqrt(varianza);

                        std::cout << "Procesados " << totalRegistrosProcesados 
                                  << " registros. Pérdida media: " << perdidaMedia
                                  << ", Desviación típica: " << desviacionTipica << std::endl;
                        loteActual = 0;

                        // Guardar el modelo periódicamente
                        if (totalRegistrosProcesados % 5000 == 0) {
                            red->save_to_json("redNeuronal.json");
                            std::cout << "Modelo guardado en redNeuronal.json" << std::endl;
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error al procesar línea: " << e.what() << "\n";
            }
        }

        // Guardar el progreso cada 1000 líneas en un hilo separado
        if (contador % 1000 == 0) {
            float perdidaMedia = totalRegistrosProcesados > 0 ? perdidaAcumulada / totalRegistrosProcesados : 0.0f;
            std::thread hiloProgreso(guardarProgreso, contador, nombreArchivo.str(), 
                                    totalRegistrosProcesados, perdidaMedia);
            hiloProgreso.detach();
        }
    }

    if (!interrupcionSolicitada) {
        float perdidaMedia = totalRegistrosProcesados > 0 ? perdidaAcumulada / totalRegistrosProcesados : 0.0f;
        std::thread hiloProgresoFinal(guardarProgreso, contador, nombreArchivo.str(), 
                                     totalRegistrosProcesados, perdidaMedia);
        hiloProgresoFinal.detach();
        
        // Guardar el modelo final
        red->save_to_json("redNeuronal.json");
        std::cout << "Entrenamiento completado. Modelo guardado en redNeuronal.json" << std::endl;
    }

    // Liberar memoria
    delete[] input;
    delete red;
    archivo.close();
    return 0;
}
//g++ -std=c++17 -O3 -g -fopenmp -march=native -o jsonExtractorJR jsonExtractorJR.cpp -lstdc++fs -pthread