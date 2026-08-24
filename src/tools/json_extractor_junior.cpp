#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <ctime>
#include <iomanip> 
#include <sstream>
#include <cstring>
#include <cassert>
#include <limits>
#include <cmath>
#include "json.hpp"
#include "chess.hpp" 
#include "neural/network_junior.hpp"
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

// [N6] Use tanh(cp/400) as objective: v = tanh(cp/400) ∈ [-1, 1].
inline float normalizarCP(int cp) {
    float valor = static_cast<float>(cp) / 400.0f;
    return std::tanh(valor);
}

// [N9] Map mate scores to a finite value: mate in N → +(MATE - N), mated in N → -(MATE - N).
static constexpr float MATE_SCORE = 30000.0f;

float parseEvaluation(const json& eval_entry) {
    // Try "cp" first.
    if (eval_entry.contains("cp") && eval_entry["cp"].is_number_integer()) {
        return static_cast<float>(eval_entry["cp"].get<int>());
    }
    // [N9] Try "mate" — map to ±(MATE - |mate|).
    if (eval_entry.contains("mate") && eval_entry["mate"].is_number_integer()) {
        int mate = eval_entry["mate"].get<int>();
        if (mate > 0) return MATE_SCORE - static_cast<float>(mate);
        if (mate < 0) return -MATE_SCORE - static_cast<float>(mate);
        return 0.0f;
    }
    return std::numeric_limits<float>::quiet_NaN();
}

void procesarLinea(const std::string& linea, std::vector<Registro>& resultados) {
    json j = json::parse(linea);

    if (!j.contains("fen") || !j["fen"].is_string() ||
        !j.contains("evals") || !j["evals"].is_array() ||
        j["evals"].empty()) {
        return;
    }

    std::string fen = j["fen"];
    const auto& eval = j["evals"][0];

    if (!eval.is_object() || !eval.contains("pvs") ||
        !eval["pvs"].is_array()) {
        return;
    }

    for (const auto& pv : eval["pvs"]) {
        if (!pv.is_object()) continue;
        // [N9] Accept both "cp" and "mate" evaluations.
        float cp = parseEvaluation(pv);
        if (std::isnan(cp)) continue;

        Registro r;
        r.fen = fen;
        r.cp = static_cast<int>(cp); // Store as int for compatibility.
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

// [N7] Encode from the perspective of the side to move: when it's black's
// turn, mirror the board vertically so that "our" pieces always advance
// upward.  The network learns half the patterns this way.
// [N8] Added en passant square (+8) and halfmove clock (+1).
constexpr int NN_INPUT_SIZE = 782;

void convertirFENaInput(const std::string& fen, float* input) {
    std::memset(input, 0, sizeof(float) * NN_INPUT_SIZE);

    chess::Board board(fen);
    const bool flip = (board.sideToMove() == chess::Color::BLACK);

    // [N7] 12 piece planes × 64 squares = 768.
    // When flip==true, mirror vertically: new_rank = 7 - rank, file stays.
    int index = 0;
    chess::PieceType types[] = {
        chess::PieceType::PAWN, chess::PieceType::KNIGHT, chess::PieceType::BISHOP,
        chess::PieceType::ROOK, chess::PieceType::QUEEN, chess::PieceType::KING
    };
    for (int pt = 0; pt < 6; ++pt) {
        for (int color = 0; color < 2; ++color) {
            chess::Color c = (color == 0) ? chess::Color::WHITE : chess::Color::BLACK;
            chess::Bitboard bb = board.pieces(types[pt], c);
            while (bb) {
                chess::Square sq = bb.pop();
                int rank = sq.rank();
                int file = sq.file();
                if (flip) rank = 7 - rank;
                input[index + rank * 8 + file] = 1.0f;
            }
            index += 64;
        }
    }

    // Castling rights (4 bits) — not flipped, they're absolute.
    auto cr = board.castlingRights();
    input[index++] = cr.has(chess::Color::WHITE, chess::Board::CastlingRights::Side::KING_SIDE) ? 1.0f : 0.0f;
    input[index++] = cr.has(chess::Color::WHITE, chess::Board::CastlingRights::Side::QUEEN_SIDE) ? 1.0f : 0.0f;
    input[index++] = cr.has(chess::Color::BLACK, chess::Board::CastlingRights::Side::KING_SIDE) ? 1.0f : 0.0f;
    input[index++] = cr.has(chess::Color::BLACK, chess::Board::CastlingRights::Side::QUEEN_SIDE) ? 1.0f : 0.0f;

    // Side to move is always encoded as 1.0 (we always encode from side-to-move perspective).
    input[index++] = 1.0f;

    // [N8] En passant square: 8 bits for file (or 0 if no en passant).
    chess::Square ep = board.enpassantSq();
    if (ep != chess::Square::NO_SQ) {
        input[index + ep.file()] = 1.0f;
    }
    index += 8;

    // [N8] Halfmove clock: single float, normalized.
    input[index++] = static_cast<float>(board.halfMoveClock()) / 100.0f;
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
        float input_init[NN_INPUT_SIZE];
        convertirFENaInput(fen, input_init);
        float resultado = red->forward(input_init);
        std::cout << "Resultado inicial: " << resultado << std::endl;
    }
    
    std::vector<Registro> registros;
    float* input = new float[NN_INPUT_SIZE];
    
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
