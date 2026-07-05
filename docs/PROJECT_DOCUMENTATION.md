# ajedrezUPV - Project Documentation

**ai Chess Engine Project**  
Developed at Universitat Politècnica de València (UPV)

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Repository Structure](#repository-structure)
3. [Chess Engine Implementation](#chess-engine-implementation)
4. [Connect Four (4 en Ratlla) AI](#connect-four-4-en-ratlla-ai)
5. [Connect Three (3 en Ratlla) Implementation](#connect-three-3-en-ratlla-implementation)
6. [Neural Network Components](#neural-network-components)
7. [Traditional Evaluation System](#traditional-evaluation-system)
8. [Data and Resources](#data-and-resources)
9. [Technologies and Dependencies](#technologies-and-dependencies)
10. [Installation and Usage](#installation-and-usage)
11. [License](#license)

---

## Project Overview

**ajedrezUPV** is an artificial intelligence project focused on chess, developed within the framework of the Universitat Politècnica de València (UPV). This chess engine aims to promote the study of decision algorithms, heuristic evaluation, and game engine programming techniques.

### Key Features

- Custom chess engine implementation
- Search algorithms and heuristic evaluation
- Modular and extensible code
- Focus on educational research and university practice
- Integration with neural networks for AI decision making
- Support for UCI (Universal Chess Interface) notation

---

## Repository Structure

```
ajedrezUPV/
├── README.md                    # Main project documentation
├── LICENSE                     # Project license (CC BY-NC 4.0)
├── docs/
│   └── FEN.pdf                 # FEN (Forsyth-Edwards Notation) documentation
│   └── PROJECT_DOCUMENTATION.md # This file
├── data/
│   ├── dataset.csv              # Training dataset
│   └── mnist_loader.py          # MNIST data loader script
├── models/
│   └── modelo.py                # Base model implementation
├── tests/
│   └── prova1.py                # Test script
├── implementations/
│   ├── alphabetaBot.cpp         # Alpha-Beta pruning bot implementation
│   ├── botConMinimaxOptimizado.cpp # Optimized Minimax bot
│   ├── NodeMove.cpp             # Move node implementation
│   ├── NodeMove.h               # Move node header
│   └── redNeuronalSquareRook.cpp # Neural network for rook movement
├── traditional_evaluation/
│   ├── EndgameEvaluator.cpp     # Endgame position evaluator
│   ├── EndgameEvaluator.h       # Endgame evaluator header
│   ├── Evaluator.cpp            # Base evaluator
│   ├── Evaluator.h              # Base evaluator header
│   ├── GeneralEvaluator.cpp     # General position evaluator
│   ├── GeneralEvaluator.h       # General evaluator header
│   ├── OpeningEvaluator.cpp     # Opening position evaluator
│   └── OpeningEvaluator.h       # Opening evaluator header
├── neural_network/
│   ├── jsonExtractor.cpp         # JSON data extractor
│   ├── json.hpp                 # JSON library header
│   └── modificacionesRedNeuronal.cpp # Neural network modifications
├── neural_network_jr/
│   ├── jsonExtractorJR.cpp      # Junior JSON extractor
│   └── redNeuronalJR.cpp         # Junior neural network
├── connect_four/
│   ├── main.ipynb               # Connect Four Jupyter notebook
│   ├── model.py                 # Connect Four AI model
│   ├── test-case-parser.py      # Test case parser
│   ├── TODO.md                  # Connect Four tasks
│   └── test/                    # Test datasets
│       ├── dataset              # Training dataset
│       ├── dataset-no-sim       # Non-similar dataset
│       ├── dataset-no-sim2      # Non-similar dataset 2
│       ├── full-test-without-score # Full test without scores
│       ├── test-complete        # Complete test
│       └── tests viejos/        # Old tests
│           ├── Test_L1_R1
│           ├── Test_L1_R2
│           ├── Test_L1_R3
│           ├── Test_L2_R1
│           ├── Test_L2_R2
│           └── Test_L3_R1
├── connect_three/
│   ├── comentaris_tres_en_ratlla.docx # Connect Three comments (Catalan)
│   ├── tres_en_ratlla1.py       # Connect Three implementation
│   └── quatre_en_ratlla.py       # Another Connect Four variant
└── lichess_db/
    ├── chunk_000.jsonl           # Lichess game database chunks
    ├── chunk_001.jsonl
    ├── ...
    └── chunk_019.jsonl
```

---

## Chess Engine Implementation

### Core Components

The chess engine is implemented primarily in C++ with the following key components:

#### 1. Alpha-Beta Bot (`implementations/alphabetaBot.cpp`)

- Implements the **Alpha-Beta pruning algorithm** for efficient game tree search
- Uses the **UCI (Universal Chess Interface)** protocol for move input
- Integrates with Disservin chess library for board representation and move generation
- Features interactive console interface for human vs. AI gameplay
- Displays chess board in ASCII format

**Key Functions:**
- `printBoard()`: Renders the chess board in console
- `isThisMoveLegal()`: Validates move legality
- `main()`: Game loop with human and AI turns

**Algorithm:**
```
Alpha-Beta Pruning:
- Maximizer: AI player (White)
- Minimizer: Human player (Black)
- Uses alpha and beta bounds to prune unnecessary branches
- Depth-limited search with iterative deepening capability
```

#### 2. Optimized Minimax Bot (`implementations/botConMinimaxOptimizado.cpp`)

- Enhanced implementation of the Minimax algorithm
- Optimized for performance with move ordering heuristics
- Supports multi-threading for parallel search

#### 3. Node and Move Management (`NodeMove.cpp`, `NodeMove.h`)

- **NodeMove** class represents a node in the game tree
- Each node stores:
  - Board state
  - List of child nodes (possible moves)
  - Evaluation score
  - Best move found
- Supports tree rebuilding and depth-limited search
- Implements `alphaBeta()` method for search
- Provides `getChildByMove()` for efficient tree traversal

---

## Connect Four (4 en Ratlla) AI

### Project: Connect Four Neural Network AI

This component implements an AI for Connect Four using neural networks.

#### Files

- **main.ipynb**: Jupyter notebook with complete implementation and training
- **model.py**: Standalone Python implementation of the Connect Four model
- **test-case-parser.py**: Utility for parsing test cases
- **test/**: Directory containing various test datasets

#### Model Architecture

```
Input Layer: 42 neurons (7x6 board flattened)
├── Hidden Layer 1: 64 neurons, tanh activation
├── Hidden Layer 2: 64 neurons, tanh activation  
├── Hidden Layer 3: 32 neurons, tanh activation
└── Output Layer: 1 neuron, linear activation

Training:
- Optimizer: Adam
- Loss Function: Mean Squared Error (MSE)
- Metrics: Mean Absolute Error (MAE)
- Batch Size: 10
- Epochs: 30
- Validation Split: 20%
- Test Split: 10%
- Early Stopping: 5 epochs patience
```

#### Key Functions

**Board Construction (`construirTablero`)**
- Creates a 7x6 board from move sequence
- Handles alternating players (1 and -1)
- Returns flattened board array

**Move Execution (`hacerTurno`)**
- Places a piece in the specified column
- Automatically finds the correct row (pieces fall to bottom)
- Handles player alternation

**Victory Check (`check_victory`)**
- Checks if a move results in victory
- Examines all four directions:
  - Horizontal (4 in a row)
  - Vertical (4 in a column)
  - Diagonal \ (4 in a diagonal)
  - Diagonal / (4 in a diagonal)
- Uses bidirectional counting for efficiency

#### Training Data

The model is trained on datasets in the `connect_four/test/` directory:
- Main training dataset: `dataset`
- Validation datasets: `dataset-no-sim`, `dataset-no-sim2`
- Full test: `full-test-without-score`
- Complete test: `test-complete`

---

## Connect Three (3 en Ratlla) Implementation

### Files

- **tres_en_ratlla1.py**: Main Connect Three implementation
- **quatre_en_ratlla.py**: Another variant (possibly Connect Four)
- **comentaris_tres_en_ratlla.docx**: Documentation in Catalan

This appears to be a simpler implementation for studying the basics of game AI before moving to the more complex Connect Four and Chess implementations.

---

## Neural Network Components

### Traditional Neural Network (`neural_network/`)

Files:
- **jsonExtractor.cpp**: Extracts data from JSON files (likely Lichess database)
- **json.hpp**: JSON library header (nlohmann/json)
- **modificacionesRedNeuronal.cpp**: Neural network modifications and enhancements

### Junior Neural Network (`neural_network_jr/`)

Files:
- **jsonExtractorJR.cpp**: Junior version of JSON extractor
- **redNeuronalJR.cpp**: Junior neural network implementation

These components are used for:
- Extracting and preprocessing chess game data
- Training neural networks on chess positions
- Implementing custom neural network architectures for move evaluation

---

## Traditional Evaluation System

### Components

The traditional evaluation system uses heuristic-based approaches to evaluate chess positions without neural networks.

#### 1. General Evaluator (`traditional_evaluation/GeneralEvaluator.cpp`)

- Evaluates general middlegame positions
- Considers material balance, piece activity, king safety
- Implements standard chess heuristics

#### 2. Opening Evaluator (`traditional_evaluation/OpeningEvaluator.cpp`)

- Specialized for opening positions
- Encourages development, center control, king safety
- Penalizes premature queen movement and multiple pawn moves

#### 3. Endgame Evaluator (`traditional_evaluation/EndgameEvaluator.cpp`)

- Specialized for endgame positions
- Considers king activity, pawn promotion, passed pawns
- Implements endgame-specific heuristics (king and pawn endgames, etc.)

#### 4. Base Evaluator (`traditional_evaluation/Evaluator.cpp`)

- Base class for all evaluators
- Provides common functionality and interface
- Coordinates between different evaluation types

---

## Data and Resources

### Lichess Database (`lichess_db/`)

- Contains 20 chunks of JSONL files (chunk_000.jsonl to chunk_019.jsonl)
- Each file contains chess games in JSON Lines format
- Used for training and evaluating the AI models
- Total dataset: Millions of chess games from Lichess platform

### Other Data Files

- **data/dataset.csv**: Training dataset (likely for Connect Four or other models)
- **data/mnist_loader.py**: Script for loading MNIST dataset (possibly for testing or comparison)

---

## Technologies and Dependencies

### Programming Languages

- **C++23**: Primary language for chess engine and evaluation components
- **Python 3.10+**: For neural network training and Connect Four implementation

### Libraries and Frameworks

#### C++ Dependencies

1. **Disservin Chess Library** (https://disservin.github.io/chess-library/)
   - Provides chess board representation
   - Implements chess rules and move generation
   - Handles FEN parsing and UCI notation
   - Used in: `implementations/`, `traditional_evaluation/`

2. **nlohmann/json** (https://github.com/nlohmann/json)
   - Modern JSON library for C++
   - Used for parsing Lichess database JSON files
   - Header file: `neural_network/json.hpp`

3. **Standard Template Library (STL)**
   - Containers, algorithms, and utilities

#### Python Dependencies

1. **TensorFlow 2.x**
   - Neural network training and inference
   - Used in: `connect_four/model.py`, `connect_four/main.ipynb`

2. **NumPy**
   - Numerical operations and array handling
   - Used for board representation and data processing

3. **scikit-learn**
   - Machine learning utilities
   - Specifically: `train_test_split` for data splitting

4. **Jupyter Notebook**
   - Interactive development environment
   - Used for `connect_four/main.ipynb`

---

## Installation and Usage

### Prerequisites

#### For C++ Components

```bash
# Install g++ with C++23 support
g++ --version  # Should be version 11 or higher

# Clone Disservin chess library (included as submodule or download separately)
# Download nlohmann/json header
```

#### For Python Components

```bash
# Create virtual environment
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install Python dependencies
pip install tensorflow numpy scikit-learn jupyter
```

### Compiling the Chess Engine

```bash
# Navigate to project directory
cd ajedrezUPV

# Compile the Alpha-Beta bot
g++ implementations/alphabetaBot.cpp \
    implementations/NodeMove.cpp \
    traditional_evaluation/GeneralEvaluator.cpp \
    traditional_evaluation/OpeningEvaluator.cpp \
    traditional_evaluation/EndgameEvaluator.cpp \
    -I. -o chess_engine

# The -I. flag adds the current directory to include path
# Make sure Disservin and json headers are available
```

### Running the Chess Engine

```bash
# Run the compiled engine
./chess_engine

# The engine will:
# 1. Display the starting chess position
# 2. Wait for your move in UCI notation (e.g., "e2e4")
# 3. Calculate and play its move using Alpha-Beta algorithm
# 4. Continue alternating until game over
```

### Training Connect Four AI

```bash
# Navigate to connect_four directory
cd connect_four

# Run the training script (using Jupyter)
jupyter notebook main.ipynb

# Or run the standalone model
python model.py

# The model will:
# 1. Load training data from test/dataset
# 2. Create and compile the neural network
# 3. Train the model
# 4. Evaluate on test set
# 5. Save the trained model
```

---

## License

This project is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International License (CC BY-NC 4.0)**.

### What This Means:

✅ **Allowed:**
- Share: Copy and redistribute the material in any medium or format
- Adapt: Remix, transform, and build upon the material
- Use for educational purposes
- Use for personal projects
- Use for research

❌ **Not Allowed:**
- Use commercially without explicit permission from the author
- Sell or profit from the project without authorization

### Attribution Requirement

You must give appropriate credit to the original author (Llorenc Perez) and indicate if changes were made.

### Full License Text

Available at: https://creativecommons.org/licenses/by-nc/4.0/

---

## Project Status

### Completed Features

- [x] Chess board representation and display
- [x] Legal move generation and validation
- [x] Alpha-Beta pruning algorithm
- [x] UCI notation support
- [x] Human vs. AI gameplay
- [x] Traditional evaluation system (Opening, General, Endgame)
- [x] Connect Four AI with neural networks
- [x] Victory condition checking
- [x] Symmetric board handling
- [x] Data loading and preprocessing

### In Progress

- [ ] Advanced neural network integration for chess
- [ ] Move explorer for Connect Four
- [ ] Enhanced evaluation heuristics
- [ ] Multi-threading optimization
- [ ] Opening book implementation

---

## Contributing

Contributions are welcome! Please contact the project author for collaboration opportunities.

**Author:** Llorenc Perez  
**Email:** perezllorenc@gmail.com  
**GitHub:** https://github.com/LLPB-pixel/ajedrezUPV

---

## Acknowledgments

- **Universitat Politècnica de València (UPV)** for academic support
- **Disservin** for the excellent chess library
- **nlohmann** for the JSON library
- **TensorFlow team** for the machine learning framework
- **Lichess** for providing the chess game database

---

## Version History

- **v1.0** (Current): Initial release with chess engine, evaluation system, and Connect Four AI
- **Future versions** will include enhanced neural network integration and additional game AI implementations

---

*Documentation generated on: July 5, 2026*  
*Project repository: https://github.com/LLPB-pixel/ajedrezUPV*
