# ajedrezUPV

**ajedrezUPV** is an artificial intelligence project focused on chess, developed within the framework of the Universitat Politècnica de València (UPV). This chess engine aims to promote the study of decision algorithms, heuristic evaluation, and game engine programming techniques.

## Features

- Custom chess engine implementation
- Search algorithms and heuristic evaluation
- Modular and extensible code
- Focus on educational research and university practice
- Neural network integration for AI decision making
- UCI (Universal Chess Interface) notation support

## Requirements

- Language: C++23
- Disservin Chess Library (https://disservin.github.io/chess-library/) - provides chess rules and board representation
- nlohmann/json (https://github.com/nlohmann/json) - for parsing JSON files from Lichess database

## Installation

You can install it using git:

```bash
git clone https://github.com/LLPB-pixel/ajedrezUPV.git
cd ajedrezUPV
```

Or manually download the files from the `implementations` directory.

In any case, you need to compile with:

```bash
g++ -std=c++23 -I. implementations/alphabetaBot.cpp implementations/NodeMove.cpp \
    traditional_evaluation/GeneralEvaluator.cpp traditional_evaluation/OpeningEvaluator.cpp \
    traditional_evaluation/EndgameEvaluator.cpp -o chess_engine
```

This will create a console application. It only accepts moves in UCI notation. (https://en.wikipedia.org/wiki/Universal_Chess_Interface)

## Repository Structure

```
ajedrezUPV/
├── README.md                    # Project documentation
├── LICENSE                      # Project license (CC BY-NC 4.0)
├── docs/
│   ├── FEN.pdf                  # FEN notation documentation
│   └── PROJECT_DOCUMENTATION.*  # Complete project documentation
├── data/
│   ├── dataset.csv              # Training datasets
│   └── mnist_loader.py          # Data loading utilities
├── models/
│   └── modelo.py                # Model implementations
├── tests/
│   └── prova1.py                # Test scripts
├── implementations/
│   ├── alphabetaBot.cpp         # Alpha-Beta pruning bot
│   ├── botConMinimaxOptimizado.cpp # Optimized Minimax bot
│   ├── NodeMove.cpp             # Move node implementation
│   ├── NodeMove.h               # Move node header
│   └── redNeuronalSquareRook.cpp # Neural network for rook movement
├── traditional_evaluation/
│   ├── EndgameEvaluator.*       # Endgame position evaluator
│   ├── Evaluator.*              # Base evaluator
│   ├── GeneralEvaluator.*       # General position evaluator
│   └── OpeningEvaluator.*       # Opening position evaluator
├── neural_network/
│   ├── jsonExtractor.cpp         # JSON data extractor
│   ├── json.hpp                 # JSON library header
│   └── modificacionesRedNeuronal.cpp # Neural network modifications
├── neural_network_jr/
│   ├── jsonExtractorJR.cpp      # Junior JSON extractor
│   └── redNeuronalJR.cpp         # Junior neural network
└── lichess_db/
    └── chunk_*.jsonl             # Lichess game database chunks
```

For any questions or bug reports, please send an email to perezllorenc@gmail.com

## Licencia
Este proyecto está licenciado bajo la [Creative Commons Attribution-NonCommercial 4.0 International License](https://creativecommons.org/licenses/by-nc/4.0/).
![Licencia](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)
