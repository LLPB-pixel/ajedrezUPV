#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <ctime>
#include <iomanip> // Para formatear números con ceros a la izquierda
#include "json.hpp"

using json = nlohmann::json;

struct Registro {
    std::string fen;
    std::string line;
    int cp;
};

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
        if (!pv.contains("line") || !pv.contains("cp")) {
            continue;
        }

        Registro r;
        r.fen = fen;
        r.line = pv["line"];
        r.cp = pv["cp"];
        resultados.push_back(std::move(r));
    }
}

void guardarProgreso(int lineaActual, const std::string& archivoActual) {
    std::ofstream archivoProgreso("progreso.txt", std::ios::app); // Abrir en modo append
    if (!archivoProgreso.is_open()) {
        std::cerr << "Error al abrir el archivo de progreso.\n";
        return;
    }

    // Obtener la hora actual
    std::time_t ahora = std::time(nullptr);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&ahora));

    // Guardar la línea actual, el archivo actual y la hora
    archivoProgreso << "Archivo: " << archivoActual << " - Línea actual: " << lineaActual << " - Hora: " << buffer << "\n";
    archivoProgreso.close();
}

int main() {
    const int totalChunks = 21; // Desde chunk_000 hasta chunk_020
    std::vector<Registro> registros;

    for (int i = 0; i < totalChunks; ++i) {
        // Formatear el nombre del archivo con ceros a la izquierda
        std::ostringstream nombreArchivo;
        nombreArchivo << "chunk_" << std::setw(3) << std::setfill('0') << i << ".jsonl";

        std::ifstream archivo(nombreArchivo.str());
        if (!archivo.is_open()) {
            std::cerr << "Error al abrir el archivo: " << nombreArchivo.str() << "\n";
            continue; // Pasar al siguiente archivo si no se puede abrir
        }

        std::string linea;
        int contador = 0;

        while (std::getline(archivo, linea)) {
            contador++;
            if (!linea.empty()) {
                try {
                    procesarLinea(linea, registros);
                    //entrenamiento de la red aquí
                    
                } catch (const std::exception& e) {
                    std::cerr << "Error al procesar línea: " << e.what() << "\n";
                }
            }

            // Guardar el progreso cada 100 líneas en un hilo separado
            if (contador % 1000 == 0) {
                std::thread hiloProgreso(guardarProgreso, contador, nombreArchivo.str());
                hiloProgreso.detach(); 
            }
        }

        std::thread hiloProgresoFinal(guardarProgreso, contador, nombreArchivo.str());
        hiloProgresoFinal.detach();

        archivo.close();
    }

    return 0;
}