"""
CNN evaluator for ajedrezUPV — replaces the 782→512→512→1 MLP with a
lightweight convolutional network that understands spatial locality.

Input:  18 × 8 × 8 planes, side-to-move perspective (board flipped for Black)
  0-5  : our P, N, B, R, Q, K
  6-11 : their P, N, B, R, Q, K
  12   : our kingside castling right  (1 plane, all 0/1)
  13   : our queenside castling right
  14   : their kingside castling right
  15   : their queenside castling right
  16   : en passant target square (1 plane, single 1)
  17   : halfmove clock (1 plane, all halfmove/100)

Architecture (light & fast, ~2.3M params, <2ms on CPU):
  Conv 3×3, 18→64,  pad=1, ReLU
  Conv 3×3, 64→128, pad=1, ReLU
  Conv 3×3, 128→128,pad=1, ReLU
  Flatten 128×8×8 = 8192
  FC 8192→256, ReLU
  FC 256→1, linear (tanh target in [-1,1])

Training: same objective as train.py — tanh(cp/400), MSE, Adam, side-to-move
perspective. Exports to .pt + .bin (for C++ cnn_network.hpp) + .json.
"""
import os
import json
import math
import random
import struct
import argparse
import concurrent.futures
from pathlib import Path
from typing import Optional, Tuple

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------
CNN_CHANNELS = 18
BOARD_SIZE = 8
BATCH_SIZE = 256
LEARNING_RATE = 1e-3
WEIGHT_DECAY = 1e-4
EPOCHS = 10
VALIDATION_SPLIT = 0.01
SEED = 42

random.seed(SEED)
np.random.seed(SEED)
torch.manual_seed(SEED)
torch.cuda.manual_seed_all(SEED)
torch.backends.cudnn.deterministic = True

# ---------------------------------------------------------------------------
# Model
# ---------------------------------------------------------------------------
class ChessCNN(nn.Module):
    """Light CNN: 3× conv(3×3) + 2× FC. No BatchNorm for C++ simplicity."""
    def __init__(self):
        super().__init__()
        self.conv1 = nn.Conv2d(CNN_CHANNELS, 64, kernel_size=3, padding=1, bias=True)
        self.conv2 = nn.Conv2d(64, 128, kernel_size=3, padding=1, bias=True)
        self.conv3 = nn.Conv2d(128, 128, kernel_size=3, padding=1, bias=True)
        self.fc1 = nn.Linear(128 * 8 * 8, 256)
        self.fc2 = nn.Linear(256, 1)
        # He init (PyTorch default for Conv2d/Linear is Kaiming uniform, close enough)
        for m in self.modules():
            if isinstance(m, (nn.Conv2d, nn.Linear)):
                nn.init.kaiming_normal_(m.weight, nonlinearity='relu')
                if m.bias is not None:
                    nn.init.zeros_(m.bias)

    def forward(self, x):
        # x: (B, 18, 8, 8)
        x = torch.relu(self.conv1(x))
        x = torch.relu(self.conv2(x))
        x = torch.relu(self.conv3(x))
        x = x.view(x.size(0), -1)  # flatten 128*8*8
        x = torch.relu(self.fc1(x))
        x = self.fc2(x).squeeze(-1)  # (B,)
        return x


# ---------------------------------------------------------------------------
# Data: same JSONL parsing as train.py
# ---------------------------------------------------------------------------
def parse_jsonl_line(line: str) -> Optional[Tuple[str, float]]:
    try:
        j = json.loads(line)
    except json.JSONDecodeError:
        return None
    if "fen" not in j or "evals" not in j:
        return None
    fen = j["fen"]
    for ev in j.get("evals", []):
        for pv in ev.get("pvs", []):
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
            target = math.tanh(cp / 400.0)
            return (fen, target)
    return None


def _fen_to_cnn_tensor_pure(fen: str) -> np.ndarray:
    """Pure-python FEN parser fallback when python-chess is not installed."""
    parts = fen.split()
    board_part = parts[0] if len(parts) > 0 else ""
    turn = parts[1] if len(parts) > 1 else "w"
    castling = parts[2] if len(parts) > 2 else "-"
    ep = parts[3] if len(parts) > 3 else "-"
    halfmove = 0
    if len(parts) > 4:
        try:
            halfmove = int(parts[4])
        except: pass
    flip = (turn == "b")
    planes = np.zeros((CNN_CHANNELS, 8, 8), dtype=np.float32)
    # Piece mapping char -> (plane offset, is_white)
    # For perspective we need to know us/them, but without chess lib we handle manually
    # We'll first build a board array [8][8] with piece char or None
    ranks = board_part.split("/")
    # FEN ranks go 8->1, our planes go 0=rank1 (white's first rank) after flip handling
    # We'll fill a temporary board representation: board[r][f] where r 0= rank1, 7=rank8 (from white perspective unflipped)
    # FEN rank 0 is rank8, so fen_r = 7 - r
    piece_map = {'P':0,'N':1,'B':2,'R':3,'Q':4,'K':5}
    us_is_white = (turn == "w")
    for fen_r, rank_str in enumerate(ranks):
        r = 7 - fen_r  # white perspective rank
        f = 0
        for ch in rank_str:
            if ch.isdigit():
                f += int(ch)
            else:
                is_white = ch.isupper()
                pt = ch.upper()
                if pt not in piece_map:
                    f+=1
                    continue
                # Determine if this piece is "ours" or "theirs" from side-to-move perspective
                is_our = (is_white == us_is_white)
                plane = piece_map[pt] + (0 if is_our else 6)
                pr = 7 - r if flip else r
                # Note: r is white perspective (0=rank1), flip if needed
                planes[plane, pr, f] = 1.0
                f+=1
    # Castling: 12:our KS,13:our QS,14:their KS,15:their QS
    # Castling string is absolute (KQkq), we map to perspective
    # K=White KS, Q=White QS, k=Black KS, q=Black QS
    has_K = "K" in castling
    has_Q = "Q" in castling
    has_k = "k" in castling
    has_q = "q" in castling
    if us_is_white:
        our_ks, our_qs, their_ks, their_qs = has_K, has_Q, has_k, has_q
    else:
        our_ks, our_qs, their_ks, their_qs = has_k, has_q, has_K, has_Q
    if our_ks: planes[12,:,:]=1.0
    if our_qs: planes[13,:,:]=1.0
    if their_ks: planes[14,:,:]=1.0
    if their_qs: planes[15,:,:]=1.0
    # En passant
    if ep != "-":
        try:
            ef = ord(ep[0]) - ord('a')
            er = int(ep[1]) - 1  # 0=rank1
            if flip: er = 7 - er
            planes[16, er, ef] = 1.0
        except: pass
    planes[17,:,:] = halfmove / 100.0
    return planes

def fen_to_cnn_tensor(fen: str) -> np.ndarray:
    """Convert FEN to 18×8×8 planes, side-to-move perspective (flip for Black)."""
    try:
        import chess
        board = chess.Board(fen)
        flip = (board.turn == chess.BLACK)
        planes = np.zeros((CNN_CHANNELS, 8, 8), dtype=np.float32)
        us = board.turn
        them = not us
        piece_order = [chess.PAWN, chess.KNIGHT, chess.BISHOP, chess.ROOK, chess.QUEEN, chess.KING]
        for i, pt in enumerate(piece_order):
            for sq in board.pieces(pt, us):
                r, f = divmod(sq, 8)
                if flip: r = 7 - r
                planes[i, r, f] = 1.0
            for sq in board.pieces(pt, them):
                r, f = divmod(sq, 8)
                if flip: r = 7 - r
                planes[6 + i, r, f] = 1.0
        our_ks = board.has_kingside_castling_rights(us)
        our_qs = board.has_queenside_castling_rights(us)
        their_ks = board.has_kingside_castling_rights(them)
        their_qs = board.has_queenside_castling_rights(them)
        if our_ks: planes[12, :, :] = 1.0
        if our_qs: planes[13, :, :] = 1.0
        if their_ks: planes[14, :, :] = 1.0
        if their_qs: planes[15, :, :] = 1.0
        if board.ep_square is not None:
            r, f = divmod(board.ep_square, 8)
            if flip: r = 7 - r
            planes[16, r, f] = 1.0
        planes[17, :, :] = board.halfmove_clock / 100.0
        return planes
    except ImportError:
        return _fen_to_cnn_tensor_pure(fen)
    except Exception:
        # Fallback to pure parser on any error
        try:
            return _fen_to_cnn_tensor_pure(fen)
        except Exception:
            raise


def _load_single_chunk(chunk_path_str: str):
    chunk_path = Path(chunk_path_str)
    cache_dir = chunk_path.parent / ".cache"
    cache_dir.mkdir(exist_ok=True)
    # CNN cache uses different suffix to avoid collision with MLP cache
    cache_path = cache_dir / f"{chunk_path.stem}.cnn.npz"

    if cache_path.exists():
        try:
            if cache_path.stat().st_mtime > chunk_path.stat().st_mtime:
                data = np.load(cache_path)
                return data["X"], data["y"], len(data["y"]), True
        except Exception:
            pass

    inputs = []
    targets = []
    with open(chunk_path, "r") as f:
        for line in f:
            result = parse_jsonl_line(line.strip())
            if result is None:
                continue
            fen, target = result
            try:
                x = fen_to_cnn_tensor(fen)
                inputs.append(x)
                targets.append(target)
            except Exception:
                continue

    if inputs:
        Xc = np.stack(inputs).astype(np.float32)  # (N, 18, 8, 8)
        yc = np.array(targets, dtype=np.float32)
        try:
            np.savez_compressed(cache_path, X=Xc, y=yc)
        except Exception:
            pass
        return Xc, yc, len(yc), False
    else:
        return np.empty((0, CNN_CHANNELS, 8, 8), dtype=np.float32), np.empty((0,), dtype=np.float32), 0, False


def load_chunks(data_dir: str, max_chunks: int = 5):
    data_path = Path(data_dir)
    chunks = sorted(data_path.glob("*.jsonl"))[:max_chunks]
    if not chunks:
        return np.empty((0, CNN_CHANNELS, 8, 8), dtype=np.float32), np.empty((0,), dtype=np.float32)

    if len(chunks) == 1:
        Xc, yc, cnt, cached = _load_single_chunk(str(chunks[0]))
        tag = " (cached)" if cached else ""
        print(f"  Loaded {cnt} positions from {chunks[0].name}{tag}")
        return Xc, yc

    max_workers = min(len(chunks), os.cpu_count() or 4, 8)
    print(f"Loading {len(chunks)} chunks with {max_workers} workers...")

    results = []
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

    results.sort(key=lambda x: x[0])
    if not results:
        return np.empty((0, CNN_CHANNELS, 8, 8), dtype=np.float32), np.empty((0,), dtype=np.float32)

    total = sum(len(r[1]) for r in results)
    X_all = np.empty((total, CNN_CHANNELS, 8, 8), dtype=np.float32)
    y_all = np.empty((total,), dtype=np.float32)
    offset = 0
    for _, Xc, yc in results:
        n = len(Xc)
        X_all[offset:offset+n] = Xc
        y_all[offset:offset+n] = yc
        offset += n
    return X_all, y_all


# ---------------------------------------------------------------------------
# Export for C++ (cnn_network.hpp)
# ---------------------------------------------------------------------------
def export_binary(model: ChessCNN, path: str):
    """Binary layout (little-endian):
       u32 version=2
       u32 channels=18, u32 H=8, W=8
       u32 conv1 out=64, u32 conv2 out=128, u32 conv3 out=128, u32 fc1 out=256
       For each conv: weight [out][in][3][3] float32, bias [out] float32
       For each FC:   weight [out][in] float32,      bias [out] float32
    """
    with open(path, "wb") as f:
        f.write(struct.pack("<I", 2))  # version 2 = CNN
        f.write(struct.pack("<IIIIII", CNN_CHANNELS, 8, 8, 64, 128, 128))
        f.write(struct.pack("<I", 256))  # fc1 out
        # conv1
        w = model.conv1.weight.detach().cpu().numpy().astype(np.float32)
        b = model.conv1.bias.detach().cpu().numpy().astype(np.float32)
        f.write(w.tobytes()); f.write(b.tobytes())
        # conv2
        w = model.conv2.weight.detach().cpu().numpy().astype(np.float32)
        b = model.conv2.bias.detach().cpu().numpy().astype(np.float32)
        f.write(w.tobytes()); f.write(b.tobytes())
        # conv3
        w = model.conv3.weight.detach().cpu().numpy().astype(np.float32)
        b = model.conv3.bias.detach().cpu().numpy().astype(np.float32)
        f.write(w.tobytes()); f.write(b.tobytes())
        # fc1
        w = model.fc1.weight.detach().cpu().numpy().astype(np.float32)
        b = model.fc1.bias.detach().cpu().numpy().astype(np.float32)
        f.write(w.tobytes()); f.write(b.tobytes())
        # fc2
        w = model.fc2.weight.detach().cpu().numpy().astype(np.float32)
        b = model.fc2.bias.detach().cpu().numpy().astype(np.float32)
        f.write(w.tobytes()); f.write(b.tobytes())
    print(f"CNN exported to {path} ({os.path.getsize(path)} bytes)")


def export_json(model: ChessCNN, path: str):
    state = model.state_dict()
    data = {
        "version": 2,
        "conv1": {"weight": state["conv1.weight"].cpu().numpy().tolist(), "bias": state["conv1.bias"].cpu().numpy().tolist()},
        "conv2": {"weight": state["conv2.weight"].cpu().numpy().tolist(), "bias": state["conv2.bias"].cpu().numpy().tolist()},
        "conv3": {"weight": state["conv3.weight"].cpu().numpy().tolist(), "bias": state["conv3.bias"].cpu().numpy().tolist()},
        "fc1":   {"weight": state["fc1.weight"].cpu().numpy().tolist(),   "bias": state["fc1.bias"].cpu().numpy().tolist()},
        "fc2":   {"weight": state["fc2.weight"].cpu().numpy().tolist(),   "bias": state["fc2.bias"].cpu().numpy().tolist()},
    }
    with open(path, "w") as f:
        json.dump(data, f)
    print(f"CNN JSON exported to {path}")


# ---------------------------------------------------------------------------
# Training
# ---------------------------------------------------------------------------
def train(args):
    X, y = load_chunks(args.data_dir, max_chunks=args.max_chunks)
    if len(X) == 0:
        print("No positions loaded — check data_dir and JSONL format.")
        return
    indices = np.random.permutation(len(X))
    X, y = X[indices], y[indices]
    split = int(len(X) * (1 - VALIDATION_SPLIT))
    X_train, y_train = X[:split], y[:split]
    X_val, y_val = X[split:], y[split:]
    print(f"\nDataset: {len(X_train)} train, {len(X_val)} validation")

    X_train_t = torch.from_numpy(X_train)
    y_train_t = torch.from_numpy(y_train)
    X_val_t = torch.from_numpy(X_val)
    y_val_t = torch.from_numpy(y_val)

    model = ChessCNN()
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE, weight_decay=WEIGHT_DECAY)
    scheduler = optim.lr_scheduler.StepLR(optimizer, step_size=2, gamma=0.5)
    criterion = nn.MSELoss()
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model.to(device)
    print(f"Using device: {device}")
    print(f"Params: {sum(p.numel() for p in model.parameters())}")

    best_val_loss = float("inf")
    for epoch in range(EPOCHS):
        model.train()
        train_loss = 0.0
        for i in range(0, len(X_train_t), BATCH_SIZE):
            xb = X_train_t[i:i+BATCH_SIZE].to(device)
            yb = y_train_t[i:i+BATCH_SIZE].to(device)
            pred = model(xb)
            loss = criterion(pred, yb)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            train_loss += loss.item() * len(xb)
        train_loss /= len(X_train_t)

        model.eval()
        with torch.no_grad():
            val_pred = model(X_val_t.to(device))
            val_loss = criterion(val_pred, y_val_t.to(device)).item()
        scheduler.step()
        print(f"Epoch {epoch+1}/{EPOCHS}  train_loss={train_loss:.6f}  val_loss={val_loss:.6f}  lr={scheduler.get_last_lr()[0]:.6f}")
        if val_loss < best_val_loss:
            best_val_loss = val_loss
            torch.save(model.state_dict(), args.output_model)
            print(f"  → Saved best model (val_loss={val_loss:.6f})")

    model.load_state_dict(torch.load(args.output_model, map_location=device))
    export_binary(model, args.output_binary)
    export_json(model, args.output_json)
    print("Training complete.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train ajedrezUPV CNN evaluator")
    parser.add_argument("--data-dir", type=str, default="lichess_db", help="Directory containing .jsonl chunks")
    parser.add_argument("--max-chunks", type=int, default=5, help="Maximum number of chunks to load")
    parser.add_argument("--output-model", type=str, default="models/cnn_best.pt", help="Path to save best checkpoint")
    parser.add_argument("--output-binary", type=str, default="models/cnn.bin", help="Path to save binary for C++")
    parser.add_argument("--output-json", type=str, default="models/cnn.json", help="Path to save JSON")
    args = parser.parse_args()
    os.makedirs("models", exist_ok=True)
    train(args)
