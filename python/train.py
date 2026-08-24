"""
[N16] PyTorch training script for ajedrezUPV neural network.
[N6]  Objective: tanh(cp/400) ∈ [-1, 1]
[N7]  Side-to-move perspective encoding.
[N8]  Extended input: 768 + 4 + 1 + 8 + 1 = 782 features.
[N9]  Mate scores handled in the C++ parser; this script reads the preprocessed data.
[N17] Reproducibility: fixed seeds, no dropout, weight decay instead.
[PERF] Multiprocess chunk loading + .npz caching + preallocated concatenation.
"""
import os
import sys
import json
import math
import random
import struct
import argparse
import concurrent.futures
from pathlib import Path
from typing import Optional

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
INPUT_SIZE = 782    # [N8]
HIDDEN1 = 512
HIDDEN2 = 512
OUTPUT_SIZE = 1

BATCH_SIZE = 256    # [N5] Mini-batches
LEARNING_RATE = 1e-3
WEIGHT_DECAY = 1e-4  # [N17] Instead of dropout.
EPOCHS = 10
VALIDATION_SPLIT = 0.01  # [N10] 1% validation set.
SEED = 42           # [N17] Fixed seed for reproducibility.

# ---------------------------------------------------------------------------
# Reproducibility [N17]
# ---------------------------------------------------------------------------
random.seed(SEED)
np.random.seed(SEED)
torch.manual_seed(SEED)
torch.cuda.manual_seed_all(SEED)
torch.backends.cudnn.deterministic = True

# ---------------------------------------------------------------------------
# Model: mirrors the C++ 773→512→512→1 architecture, but with 782 inputs.
# ---------------------------------------------------------------------------
class ChessNet(nn.Module):
    def __init__(self):
        super().__init__()
        self.fc1 = nn.Linear(INPUT_SIZE, HIDDEN1)
        self.fc2 = nn.Linear(HIDDEN1, HIDDEN2)
        self.fc3 = nn.Linear(HIDDEN2, OUTPUT_SIZE)
        # [N1] He initialization (PyTorch default for Linear is already He).
        # [N17] No dropout — use weight decay instead.

    def forward(self, x):
        x = torch.relu(self.fc1(x))
        x = torch.relu(self.fc2(x))
        x = self.fc3(x)  # Linear output — tanh target is in [-1, 1].
        return x.squeeze(-1)


# ---------------------------------------------------------------------------
# Data loading from JSONL chunks (Lichess engine evaluations).
# ---------------------------------------------------------------------------
def parse_jsonl_line(line: str) -> Optional[tuple]:
    """Parse a single JSONL line into (fen, cp_value)."""
    try:
        j = json.loads(line)
    except json.JSONDecodeError:
        return None

    if "fen" not in j or "evals" not in j:
        return None

    fen = j["fen"]
    for ev in j.get("evals", []):
        for pv in ev.get("pvs", []):
            # [N9] Accept both "cp" and "mate" evaluations.
            if "cp" in pv and isinstance(pv["cp"], int):
                cp = float(pv["cp"])
            elif "mate" in pv and isinstance(pv["mate"], int):
                mate = pv["mate"]
                if mate > 0:
                    cp = 30000.0 - float(mate)
                elif mate < 0:
                    cp = -30000.0 - float(mate)
                else:
                    continue
            else:
                continue

            # [N6] Target: tanh(cp/400) ∈ [-1, 1].
            target = math.tanh(cp / 400.0)
            return (fen, target)
    return None


def fen_to_tensor(fen: str) -> np.ndarray:
    """Convert FEN to 782-dimensional input vector.
    [N7] Encoded from side-to-move perspective (mirror board for black).
    [N8] Includes en passant and halfmove clock.
    """
    import chess
    board = chess.Board(fen)
    flip = (board.turn == chess.BLACK)

    x = np.zeros(INPUT_SIZE, dtype=np.float32)
    idx = 0

    piece_types = [chess.PAWN, chess.KNIGHT, chess.BISHOP,
                   chess.ROOK, chess.QUEEN, chess.KING]
    for pt in piece_types:
        for color in [chess.WHITE, chess.BLACK]:
            for sq in board.pieces(pt, color):
                rank = sq // 8
                f = sq % 8
                if flip:
                    rank = 7 - rank
                x[idx + rank * 8 + f] = 1.0
            idx += 64

    # Castling rights (4 bits, absolute — not flipped).
    x[idx]     = float(board.has_kingside_castling_rights(chess.WHITE))
    x[idx + 1] = float(board.has_queenside_castling_rights(chess.WHITE))
    x[idx + 2] = float(board.has_kingside_castling_rights(chess.BLACK))
    x[idx + 3] = float(board.has_queenside_castling_rights(chess.BLACK))
    idx += 4

    # Side to move — always 1.0 (we encode from our perspective).
    x[idx] = 1.0
    idx += 1

    # En passant file (8 bits).
    if board.ep_square is not None:
        ep_file = board.ep_square % 8
        x[idx + ep_file] = 1.0
    idx += 8

    # Halfmove clock (normalized).
    x[idx] = board.halfmove_clock / 100.0
    idx += 1

    return x


def _load_single_chunk(chunk_path_str: str):
    """Worker: load one JSONL chunk, with .npz caching."""
    chunk_path = Path(chunk_path_str)
    cache_dir = chunk_path.parent / ".cache"
    cache_dir.mkdir(exist_ok=True)
    cache_path = cache_dir / f"{chunk_path.stem}.npz"

    # [PERF] Use cached .npz if it exists and is newer than the source.
    if cache_path.exists():
        try:
            if cache_path.stat().st_mtime > chunk_path.stat().st_mtime:
                data = np.load(cache_path)
                Xc, yc = data["X"], data["y"]
                return Xc, yc, len(yc), True
        except Exception:
            pass  # fall through to recompute

    inputs: list = []
    targets: list = []
    with open(chunk_path, "r") as f:
        for line in f:
            result = parse_jsonl_line(line.strip())
            if result is None:
                continue
            fen, target = result
            try:
                x = fen_to_tensor(fen)
                inputs.append(x)
                targets.append(target)
            except Exception:
                continue

    if inputs:
        Xc = np.array(inputs, dtype=np.float32)
        yc = np.array(targets, dtype=np.float32)
        try:
            np.savez_compressed(cache_path, X=Xc, y=yc)
        except Exception:
            pass
        return Xc, yc, len(yc), False
    else:
        return np.empty((0, INPUT_SIZE), dtype=np.float32), np.empty((0,), dtype=np.float32), 0, False


def load_chunks(data_dir: str, max_chunks: int = 5):
    """Load JSONL chunks and return (inputs, targets) as numpy arrays.

    [PERF] Parallel chunk loading with ProcessPoolExecutor and .npz caching.
    Chunks are loaded in parallel, cached as compressed .npz files, and
    concatenated via preallocated arrays (no repeated list resizing).
    """
    data_path = Path(data_dir)
    chunks = sorted(data_path.glob("*.jsonl"))[:max_chunks]
    if not chunks:
        return np.empty((0, INPUT_SIZE), dtype=np.float32), np.empty((0,), dtype=np.float32)

    # Single chunk — no pool overhead.
    if len(chunks) == 1:
        Xc, yc, cnt, cached = _load_single_chunk(str(chunks[0]))
        tag = " (cached)" if cached else ""
        print(f"  Loaded {cnt} positions from {chunks[0].name}{tag}")
        return Xc, yc

    max_workers = min(len(chunks), os.cpu_count() or 4, 8)
    print(f"Loading {len(chunks)} chunks with {max_workers} workers...")

    results: list = []
    with concurrent.futures.ProcessPoolExecutor(max_workers=max_workers) as executor:
        future_to_chunk = {executor.submit(_load_single_chunk, str(c)): c for c in chunks}
        for future in concurrent.futures.as_completed(future_to_chunk):
            chunk = future_to_chunk[future]
            try:
                Xc, yc, cnt, cached = future.result()
                tag = " (cached)" if cached else ""
                print(f"  Loaded {cnt} positions from {chunk.name}{tag}")
                results.append((chunk.name, Xc, yc))
            except Exception as e:
                print(f"  Failed to load {chunk.name}: {e}")

    # Restore original sorted order for determinism.
    results.sort(key=lambda x: x[0])
    if not results:
        return np.empty((0, INPUT_SIZE), dtype=np.float32), np.empty((0,), dtype=np.float32)

    # [PERF] Preallocate final arrays and copy.
    total = sum(len(r[1]) for r in results)
    X_all = np.empty((total, INPUT_SIZE), dtype=np.float32)
    y_all = np.empty((total,), dtype=np.float32)
    offset = 0
    for _, Xc, yc in results:
        n = len(Xc)
        X_all[offset:offset + n] = Xc
        y_all[offset:offset + n] = yc
        offset += n

    return X_all, y_all


# ---------------------------------------------------------------------------
# Export to binary format for C++ inference [N14].
# ---------------------------------------------------------------------------
def export_binary(model: ChessNet, path: str):
    """Export model weights as binary float32 arrays with version header."""
    with open(path, "wb") as f:
        # Version: 1
        f.write(struct.pack("<I", 1))
        # Layer sizes
        f.write(struct.pack("<I", INPUT_SIZE))
        f.write(struct.pack("<I", HIDDEN1))
        f.write(struct.pack("<I", HIDDEN2))
        f.write(struct.pack("<I", OUTPUT_SIZE))
        # Weights and biases for each layer
        for layer in [model.fc1, model.fc2, model.fc3]:
            w = layer.weight.detach().cpu().numpy().astype(np.float32)
            b = layer.bias.detach().cpu().numpy().astype(np.float32)
            f.write(w.tobytes())
            f.write(b.tobytes())
    print(f"Model exported to {path} ({os.path.getsize(path)} bytes)")


# ---------------------------------------------------------------------------
# Training loop
# ---------------------------------------------------------------------------
def train(args):
    # Load data
    X, y = load_chunks(args.data_dir, max_chunks=args.max_chunks)

    # [N10] Shuffle data globally.
    indices = np.random.permutation(len(X))
    X, y = X[indices], y[indices]

    # [N10] Split into train/validation.
    split = int(len(X) * (1 - VALIDATION_SPLIT))
    X_train, y_train = X[:split], y[:split]
    X_val, y_val = X[split:], y[split:]

    print(f"\nDataset: {len(X_train)} train, {len(X_val)} validation")

    # Convert to PyTorch tensors
    X_train_t = torch.from_numpy(X_train)
    y_train_t = torch.from_numpy(y_train)
    X_val_t = torch.from_numpy(X_val)
    y_val_t = torch.from_numpy(y_val)

    # Create model and optimizer
    model = ChessNet()
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE, weight_decay=WEIGHT_DECAY)
    scheduler = optim.lr_scheduler.StepLR(optimizer, step_size=2, gamma=0.5)
    criterion = nn.MSELoss()

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model.to(device)
    print(f"Using device: {device}")

    best_val_loss = float("inf")

    for epoch in range(EPOCHS):
        model.train()
        train_loss = 0.0
        n_batches = 0

        # [N5] Mini-batch training.
        for i in range(0, len(X_train_t), BATCH_SIZE):
            xb = X_train_t[i:i+BATCH_SIZE].to(device)
            yb = y_train_t[i:i+BATCH_SIZE].to(device)

            pred = model(xb)
            loss = criterion(pred, yb)

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

            train_loss += loss.item() * len(xb)
            n_batches += 1

        train_loss /= len(X_train_t)

        # Validation
        model.eval()
        with torch.no_grad():
            val_pred = model(X_val_t.to(device))
            val_loss = criterion(val_pred, y_val_t.to(device)).item()

        scheduler.step()

        print(f"Epoch {epoch+1}/{EPOCHS}  "
              f"train_loss={train_loss:.6f}  val_loss={val_loss:.6f}  "
              f"lr={scheduler.get_last_lr()[0]:.6f}")

        if val_loss < best_val_loss:
            best_val_loss = val_loss
            # Save best model
            torch.save(model.state_dict(), args.output_model)
            print(f"  → Saved best model (val_loss={val_loss:.6f})")

    # [N14] Export to binary for C++ inference.
    model.load_state_dict(torch.load(args.output_model))
    export_binary(model, args.output_binary)

    # Save as JSON too for backward compatibility.
    export_json(model, args.output_json)
    print("Training complete.")


def export_json(model: ChessNet, path: str):
    """Export model as JSON (backward compatible with C++ loader)."""
    state = model.state_dict()
    data = {
        "layer1": {
            "weights": state["fc1.weight"].cpu().numpy().tolist(),
            "biases": state["fc1.bias"].cpu().numpy().tolist(),
        },
        "layer2": {
            "weights": state["fc2.weight"].cpu().numpy().tolist(),
            "biases": state["fc2.bias"].cpu().numpy().tolist(),
        },
        "output": {
            "weights": state["fc3.weight"].cpu().numpy().tolist(),
            "biases": state["fc3.bias"].cpu().numpy().tolist(),
        },
        "input_size": INPUT_SIZE,
    }
    with open(path, "w") as f:
        json.dump(data, f)
    print(f"Model exported to {path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train ajedrezUPV neural network")
    parser.add_argument("--data-dir", type=str, default="lichess_db",
                        help="Directory containing .jsonl chunks")
    parser.add_argument("--max-chunks", type=int, default=5,
                        help="Maximum number of chunks to load")
    parser.add_argument("--output-model", type=str, default="models/chess_net_best.pt",
                        help="Path to save best model checkpoint")
    parser.add_argument("--output-binary", type=str, default="models/chess_net.bin",
                        help="Path to save binary model for C++ inference")
    parser.add_argument("--output-json", type=str, default="models/chess_net.json",
                        help="Path to save JSON model for C++ loader")
    args = parser.parse_args()

    os.makedirs("models", exist_ok=True)
    train(args)
