# ajedrezUPV

**ajedrezUPV** es un motor de ajedrez con inteligencia artificial, desarrollado en la Universitat Politècnica de València (UPV). Combina búsqueda alpha-beta con técnicas avanzadas (profundización iterativa, tabla de transposición, quiescence search, ordenación de movimientos) y redes neuronales para la evaluación de posiciones.

## Características

- Motor de ajedrez en C++17 con librería [Disservin chess-library](https://disservin.github.io/chess-library/)
- **Búsqueda avanzada**: Alpha-Beta con profundización iterativa, aspiration windows, tabla de transposición (4-way set-associative), quiescence search, ordenación de movimientos (TT move, MVV-LVA, killers, history heuristic)
- **Evaluación heurística**: material con mapas de calor, estructura de peones (aislados, doblados, pasados), seguridad del rey (fracción de casillas vecinas bajo ataque enemigo), control del tablero, detección correcta de jaque mate y tablas
- **Redes neuronales** (C++): red grande (2565→2048→1536→1024→768→512→256→1) y red junior (782→512→512→1) con inicialización He, pérdida MSE, y serialización JSON/binaria
- **Entrenamiento PyTorch**: script de entrenamiento con mini-batches, perspectiva del lado al mueve, objetivo tanh(cp/400), exportación a binario para inferencia C++
- Protocolo de juego interactivo con comandos: movimientos UCI, `undo`, `new`, `fen`, `d`, `go depth N`
- Script `play.sh` para jugar fácilmente
- **52 tests** automatizados (unitarios, perft, mate-in-N, end-to-end)

## Compilación

### Con Makefile (rápido)

```bash
make all          # Compila motor + herramientas neuronales
make engine       # Solo el motor
make neural       # Solo herramientas neuronales
make test         # Compila y ejecuta todos los tests (52 tests)
make clean        # Limpia binarios
```

### Con CMake (recomendado)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
cd build && ctest  # Ejecuta todos los tests
```

## Jugar contra el motor

### Opción rápida (script)

```bash
./play.sh                     # Partida normal (profundidad 3)
./play.sh --build             # Compilar primero + jugar
./play.sh --depth 5           # Profundidad 5
./play.sh --fen "FEN"         # Desde una posición FEN
./play.sh --help              # Ver ayuda completa
```

### Directamente

```bash
./bin/alphabeta_optimized                         # Profundidad 3
./bin/alphabeta_optimized --depth 5               # Profundidad 5
./bin/alphabeta_optimized --fen "rnbqkbnr/..."    # Posición FEN
./bin/alphabeta_optimized --help                  # Ayuda
```

### Comandos durante la partida

| Comando | Descripción |
|---------|-------------|
| `e2e4` | Mover (notación UCI) |
| `e7e8q` | Promoción de peón |
| `e8g8` | Enroque |
| `d` | Mostrar tablero |
| `fen` | Mostrar FEN actual |
| `undo` | Deshacer últimos 2 movimientos (tuyo + del motor) |
| `new` | Nueva partida |
| `go depth 5` | Motor juega con profundidad 5 |
| `quit` / `q` / `exit` | Salir |

## Tests

```bash
make test     # 52 tests (unitarios + end-to-end)
```

### Suite de tests

| Suite | Tests | Qué verifica |
|-------|-------|-------------|
| test_node_move | 3 | Árbol de nodos, rebuild, alpha-beta con restauración de tablero |
| test_search_benchmark | 1 | Benchmark de búsqueda depth 1-3 (construcción + alpha-beta) |
| test_json_extractor | 3 | Parser de evaluaciones JSONL de Lichess |
| test_json_extractor_junior | 3 | Conversión FEN a tensor de entrada (782 features) |
| test_phase2 | 13 | TT (store/probe/replace/mate-score), búsqueda on-demand, profundización iterativa, perft (startpos, kiwipete, position3), mate-in-1 y mate-in-2 |
| test_e2e | 29 | End-to-end: arranque, movimientos, validación de entrada, EOF, comandos (quit/d/fen/new/undo/go), reglas de ajedrez (enroque, en passant, promoción), jaque mate |

## Estructura del repositorio

```
ajedrezUPV/
├── CMakeLists.txt                    # Build CMake (con FetchContent para chess-library)
├── Makefile                          # Build Make
├── play.sh                           # Script para jugar fácilmente
├── requirements.txt                  # Dependencias Python (numpy, pandas, matplotlib, python-chess)
├── include/chess/
│   ├── evaluator.h                   # Interfaz base de evaluación + constantes (eval:: namespace)
│   ├── general_evaluator.h           # Evaluador general con métodos comunes
│   ├── opening_evaluator.h           # Evaluador de apertura (mapas de calor de piezas menores, rey)
│   ├── endgame_evaluator.h           # Evaluador de finales (rey centralizado, peones avanzados)
│   ├── node_move_optimized.h         # Nodo del árbol + SearchResult + quiescence (legacy)
│   ├── transposition_table.h         # Tabla de transposición (4-way set-associative)
│   └── search.h                      # Búsqueda on-demand con TT + ID + aspiration windows
├── src/
│   ├── engine/
│   │   ├── alphabeta_optimized.cpp   # Motor principal (bucle de juego, comandos)
│   │   ├── node_move_optimized.cpp   # Árbol optimizado con alpha-beta + quiescence (legacy)
│   │   ├── transposition_table.cpp   # Implementación de la tabla de transposición
│   │   ├── search.cpp                # Búsqueda on-demand (TT + ID + quiescence + killers)
│   │   └── evaluation/
│   │       ├── general_evaluator.cpp  # Evaluación: material, peones, seguridad rey, control
│   │       ├── opening_evaluator.cpp  # Evaluador de apertura (con mapas de calor)
│   │       └── endgame_evaluator.cpp  # Evaluador de finales (rey centralizado)
│   ├── neural/
│   │   ├── network_junior.hpp         # Red junior (782→512→512→1) con inicialización He
│   │   ├── square_rook_network.cpp    # Red grande (2565→2048→...→1, ~14M params)
│   │   └── simd_network_wip.cpp      # Prototipo SIMD (AVX2, WIP)
│   └── tools/
│       ├── json_extractor.cpp         # Extractor de evaluaciones JSONL
│       └── json_extractor_junior.cpp  # Extractor + entrenador inline con perspectiva del turno
├── python/
│   ├── train.py                       # Entrenamiento PyTorch (mini-batches, tanh, export binario)
│   ├── model.py                       # Análisis de datasets y visualización
│   ├── network_reference.py           # Referencia de red neuronal (Michael Nielsen)
│   └── mnist_loader.py               # Cargador MNIST (referencia)
├── test/                              # 52 tests automatizados
├── data/dataset.csv                   # Dataset de análisis
├── lichess_db/                        # Chunks JSONL de evaluaciones de Lichess
├── third_party/                       # json.hpp, chess.hpp
├── docs/                              # Documentación LaTeX y PDFs
└── LICENSE                            # CC BY-NC 4.0
```

## Arquitectura del motor

### Búsqueda

El motor utiliza dos implementaciones de búsqueda:

1. **`Search`** (recomendada): Búsqueda on-demand con generación lazy de movimientos. Implementa:
   - Profundización iterativa con aspiration windows
   - Tabla de transposición 4-way set-associative (16 MB por defecto)
   - Quiescence search con stand-pat y MVV-LVA
   - Ordenación: TT move → capturas MVV-LVA → promociones → killers → history
   - Detección de jaque mate, ahogado, tablas (50 movimientos, repetición, material insuficiente)
   - Control de tiempo para futura integración UCI

2. **`NodeMoveOptimized`** (legacy): Árbol pre-construido con arena PMR. Mismas técnicas de búsqueda pero con el árbol materializado en memoria.

### Evaluación

- **Escala**: Material en centipeones (peon=100, caballo=300, alfil=315, torre=500, dama=900)
- **Fases**: Detección por presencia de damas → `OpeningEvaluator` o `EndgameEvaluator`
- **Términos**: Material + mapas de calor, estructura de peones (aislados, doblados, pasados por todas las filas), seguridad del rey (fracción de casillas vecinas bajo ataque), control del tablero
- **Perspectiva**: Negamax (todos los términos en perspectiva del lado al mueve)

### Red neuronal

- **Red junior** (`network_junior.hpp`): 782→512→512→1 con He init, MSE loss, Adam optimizer
- **Red grande** (`square_rook_network.cpp`): 2565→2048→1536→1024→768→512→256→1 (~14M params)
- **Entrenamiento PyTorch** (`python/train.py`): Mini-batches, tanh(cp/400), weight decay, exportación binaria
- **Encoding**: 768 (piezas) + 4 (enroque) + 1 (turno) + 8 (en passant) + 1 (reloj 50 mov) = 782 features, perspectiva del lado al mueve

## Estado de las mejoras

### Completadas

| ID | Mejora | Estado |
|----|--------|--------|
| B1 | Puntuación correcta de mate y tablas | ✅ `isCheckmate()`, `isStalemate()`, `MATE_SCORE - ply` |
| B2 | Quiescence search | ✅ Stand-pat, capturas/promociones, MVV-LVA, profundidad 8 |
| B3 | Ordenación de movimientos | ✅ TT move, MVV-LVA, killers (2/ply), history heuristic |
| B4 | Tabla de transposición | ✅ 4-way set-associative, flags EXACT/LOWER/UPPER, ajuste ply |
| B5 | Profundización iterativa | ✅ ID + aspiration windows (depth ≥ 4) |
| B6 | Árbol bajo demanda | ✅ `Search` genera movimientos lazy con make/unmake |
| B8 | SearchResult estructurado | ✅ `{move, score, depth}` en vez de comparación de floats |
| E1 | Corrección GameResult | ✅ `LOSE` en vez de `WIN` para jaque mate |
| E2 | Corrección control() | ✅ Devuelve score para el color pedido |
| E3 | Bounds en pawn_structure | ✅ `file > 0` / `file < 7` antes de acceder |
| E4 | Peón pasado completo | ✅ Comprueba todas las filas hacia adelante |
| E5 | Eliminar std::clamp | ✅ Penalties fluyen sin recorte |
| E7 | Corrección safe_king | ✅ Fracción de casillas vecinas bajo ataque |
| E9 | Eliminar std::async | ✅ Evaluación secuencial |
| N1 | Inicialización He | ✅ `sqrt(2/IN)`, Adam moments a 0 |
| N2 | Gradiente coherente | ✅ MSE puro (gradiente = pred - target) |
| N5 | Mini-batches | ✅ PyTorch: batch_size=256 |
| N6 | Función objetivo | ✅ `tanh(cp/400)` en [-1, 1] |
| N7 | Perspectiva del turno | ✅ Tablero espejado para negras |
| N8 | Encoding extendido | ✅ 782 features (con en passant + halfmove clock) |
| N9 | Mate scores en parser | ✅ `±(MATE - N)` en extractor C++ y Python |
| N14 | Formato binario de pesos | ✅ Exportación con cabecera de versión en train.py |
| N16 | Entrenar en PyTorch | ✅ Script completo con validación |
| N17 | Reproducibilidad | ✅ Seeds fijos, weight decay sin dropout |
| Q3 | Higiene del repositorio | ✅ .gitignore completo, .o eliminados de src/ |
| Q4 | Avisos de compilación | ✅ `-Wall -Wextra` en Makefile y CMake |

### Pendientes

| ID | Mejora | Prioridad |
|----|--------|-----------|
| B7 | Podas avanzadas (null-move, LMR, futility) | Alta |
| B9 | Límite de memoria en vez de MAX_BRANCH | Media |
| B10 | Limpieza API del explorador | Baja |
| E6 | Activar pawn_structure() en evaluadores | Alta |
| E10 | Evaluación interpolada por fases (tapered) | Media |
| E11 | Escala completa en centipeones | Media |
| E12 | Términos adicionales (movilidad, par de alfiles, torres en columna) | Media |
| E13 | Ajuste automático de pesos (Texel tuning) | Baja |
| E14 | Higiene de cabeceras (eliminación de `using namespace` en .h) | Baja |
| I1 | NeuralEvaluator como Evaluator | Alta |
| I2 | Híbrido por fase (red + heurístico) | Media |
| I3 | Medición de fuerza (cutechess-cli, SPRT) | Alta |
| N10 | Higiene del dataset (shuffle, deduplicación, balanceo) | Media |
| N11 | Arquitectura NNUE (acumulador incremental) | Baja |
| N12 | Simplificar red grande | Baja |
| N13 | Terminar prototipo SIMD | Baja |
| N15 | Cuantización int8 | Baja |
| P1 | Lazy SMP | Baja |
| P2 | Flags `-O3 -march=native` | Baja |
| P3 | Paralelismo correcto en entrenamiento | Baja |
| U1 | Protocolo UCI completo | Media |
| U2 | Gestión de reloj completa | Media |
| U4 | Registro de análisis (--log) | Baja |
| A1 | Libro de aperturas | Baja |
| Q1 | Integración continua (GitHub Actions) | Media |
| Q2 | Sanitizadores en CI | Baja |
| Q5 | Más tests automatizados | Media |

## Tecnologías

- **C++17**: motor de búsqueda, evaluadores, redes neuronales
- **Librerías**: [Disservin chess-library](https://disservin.github.io/chess-library/), nlohmann/json, OpenMP
- **Python**: PyTorch (entrenamiento), NumPy, pandas, matplotlib, python-chess
- **Build**: CMake (recomendado) o Makefile
- **Tests**: Framework propio con 52 tests automatizados

## Datos

El proyecto incluye chunks JSONL de evaluaciones de Lichess (`lichess_db/`) utilizados para entrenar las redes neuronales. El extractor de datos (`json_extractor_junior.cpp`) convierte FEN a tensores de entrada y normaliza evaluaciones con `tanh(cp/400)`.

Para usar el entrenamiento PyTorch:

```bash
pip install -r requirements.txt torch
python python/train.py --data-dir lichess_db --max-chunks 5
```

Los modelos se exportan en tres formatos:
- `.pt` (PyTorch checkpoint)
- `.bin` (binario float32 para inferencia C++)
- `.json` (compatibilidad con loader C++)

## Licencia

Este proyecto está licenciado bajo la [Creative Commons Attribution-NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).
![Licencia](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)

## Contacto

Para cualquier duda o sugerencia: perezllorenc@gmail.com
