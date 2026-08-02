# ajedrezUPV

**ajedrezUPV** es un proyecto de inteligencia artificial centrado en el ajedrez, desarrollado en el marco de la Universitat Politècnica de València (UPV). Este motor de ajedrez estudia algoritmos de decisión, evaluación heurística y técnicas de programación de motores de juego.

## Características

- Motor de ajedrez en C++ (con la librería [Disservin chess-library](https://disservin.github.io/chess-library/))
- Algoritmos de búsqueda: Minimax y poda Alpha-Beta (paralelizado)
- Evaluación heurística: material, estructura de peones, seguridad de rey y control
- Redes neuronales (C++) para la toma de decisiones
- Soporte de notación UCI ([Universal Chess Interface](https://en.wikipedia.org/wiki/Universal_Chess_Interface))
- Herramientas Python para análisis de datasets de Lichess

## Estructura del repositorio

```
ajedrezUPV/
├── CMakeLists.txt            # Build con CMake (descarga chess-library automáticamente)
├── Makefile                  # Build alternativo con g++ (make setup descarga la librería)
├── include/chess/            # Cabeceras públicas del motor
│   ├── node_move.h           #   Nodo del árbol de juego (minimax/alpha-beta)
│   ├── evaluator.h           #   Interfaz base de evaluación
│   ├── general_evaluator.h   #   Evaluador general
│   ├── opening_evaluator.h   #   Evaluador de apertura
│   └── endgame_evaluator.h   #   Evaluador de finales
├── src/
│   ├── engine/               # Motor de juego
│   │   ├── alphabeta_bot.cpp #   Bot principal (poda Alpha-Beta)
│   │   ├── minimax_bot.cpp   #   Bot con Minimax
│   │   ├── node_move.cpp     #   Implementación del árbol de juego
│   │   └── evaluation/       #   Evaluadores (implementación)
│   ├── neural/               # Redes neuronales en C++
│   │   ├── square_rook_network.cpp  #   Red grande (input 2565)
│   │   ├── network_junior.hpp       #   Red junior (entrenamiento con datos Lichess)
│   │   └── simd_network_wip.cpp     #   Prototipo SIMD (experimental, no compila aún)
│   └── tools/                # Herramientas de datos
│       ├── json_extractor.cpp        #   Extrae FEN/evaluaciones de chunks JSONL
│       └── json_extractor_junior.cpp #   Entrenador online sobre datos JSONL
├── python/                   # Herramientas Python
│   ├── model.py              #   Análisis del dataset y planos FEN
│   ├── mnist_loader.py       #   Utilidades de carga de datos
│   └── network_reference.py  #   Implementación de referencia (SGD/backprop)
├── data/                     # Datasets de entrenamiento
│   └── dataset.csv
├── lichess_db/               # Base de datos de partidas de Lichess (chunks JSONL)
├── third_party/              # Librerías de terceros
│   └── json.hpp              #   nlohmann/json (header-only, vendored)
├── docs/                     # Documentación (FEN.pdf, PROJECT_DOCUMENTATION)
├── .gitignore                # Archivos ignorados (bin/, build/, third_party/chess.hpp)
├── requirements.txt          # Dependencias Python
└── LICENSE                   # CC BY-NC 4.0
```

## Requisitos

- Compilador C++17 (g++ o clang++)
- [OpenMP](https://www.openmp.org/) (paralelización)
- **Alternativa A (CMake):** CMake ≥ 3.16. Descarga automáticamente la librería chess-library en la primera configuración.
- **Alternativa B (Makefile):** `make setup` descarga `chess.hpp` a `third_party/`.
- Python 3 con `pip install -r requirements.txt` (herramientas Python).

## Compilación

### Con CMake (recomendado)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Los ejecutables quedan en `build/` (`alphabeta_bot`, `minimax_bot`, `square_rook_network`, `json_extractor`, `json_extractor_junior`).

### Con Makefile

```bash
make all        # o: make engine / make neural
```

Los binarios quedan en `bin/`.

## Uso

```bash
./bin/alphabeta_bot        # Jugar contra el motor (notación UCI, ej: e2e4)
./bin/minimax_bot
```

## Estado

- [x] Motor de ajedrez, Minimax y Alpha-Beta
- [x] Sistema de evaluación (apertura/final)
- [x] Comprobación de victoria
- [ ] Red neuronal avanzada para ajedrez
- [ ] Explorador de movimientos
- [ ] Multi-hilo avanzado
- [ ] Libro de aperturas

Para cualquier duda o sugerencia: perezllorenc@gmail.com

## Licencia
Este proyecto está licenciado bajo la [Creative Commons Attribution-NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).
![Licencia](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)
