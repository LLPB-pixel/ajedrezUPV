import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import chess
from pathlib import Path

DATASET_PATH = Path(__file__).resolve().parent.parent / "data" / "dataset.csv"


print("Loading data...")
df = pd.read_csv(DATASET_PATH)
print("Data loaded successfully.")

df_numeric = df[~df["score"].astype(str).str.contains("mate")].copy()
df_numeric["score"] = pd.to_numeric(df_numeric["score"])

min_val = df_numeric["score"].min()
max_val = df_numeric["score"].max()


df_numeric[(df_numeric["score"] >= -1000) & (df_numeric["score"] <= 1000) & (df_numeric["score"]!=0)]["score"].hist(bins=1000, figsize=(8, 6), alpha=0.7)
plt.show()

def parse_score(s):
    s = str(s)
    if "mate" in s:
        sign = -1 if "-" in s else 1
        return sign * 1000 #añadimos mate a los extremos. no hay mucha diferencia entre un +10 y mate en x
    else:
        try:
            return float(s)
        except:
            return np.nan

# Aplicamos la limpieza
df["score"] = df["score"].apply(parse_score)

# Eliminamos valores faltantes (NaN)
df_clean = df.dropna(subset=["score"])

# --- Mostramos tabla resumen ---
print("\nDatos limpios (primeras 10 filas):")
print(df_clean.head(10))

# --- Calculamos estadísticas descriptivas ---
print("\nResumen estadístico de la columna 'score':")
print(df_clean["score"].describe())

# --- Visualizamos la distribución ---
plt.figure(figsize=(8, 6))
plt.hist(
    df_clean["score"],
    bins=1000,
    alpha=0.7,
    color="steelblue",
    range=(-1000, 1000)
)
plt.title("Distribución de 'score' después de limpieza")
plt.xlabel("Score")
plt.ylabel("Frecuencia")
plt.grid(True, alpha=0.3)
plt.show()

df_train = df_clean[(df_clean["score"] > -999) & (df_clean["score"] < 999)]

plt.figure(figsize=(8, 6))
plt.hist(
    df_train["score"],
    bins=1000,
    alpha=0.7,
    color="steelblue",
    range=(-1000, 1000)
)
plt.title("Distribución de 'score' después de eliminar mates")
plt.xlabel("Score")
plt.ylabel("Frecuencia")
plt.grid(True, alpha=0.3)
plt.show()


def fen_to_planes(fen: str) -> np.ndarray:

    board = chess.Board(fen)
    planes = np.zeros((8, 8, 17), dtype=np.float32)

    # --- Pieces ---
    piece_map = board.piece_map()
    for square, piece in piece_map.items():
        piece_type = piece.piece_type - 1  # 0-5
        color = 0 if piece.color == chess.WHITE else 6
        plane = piece_type + color
        row = 7 - (square // 8)
        col = square % 8
        planes[row, col, plane] = 1.0

    # --- Side to move ---
    if board.turn == chess.WHITE:
        planes[:, :, 12] = 1.0  # full plane of 1s for white to move

    # --- Castling rights ---
    if board.has_kingside_castling_rights(chess.WHITE):
        planes[:, :, 13] = 1.0
    if board.has_queenside_castling_rights(chess.WHITE):
        planes[:, :, 14] = 1.0
    if board.has_kingside_castling_rights(chess.BLACK):
        planes[:, :, 15] = 1.0
    if board.has_queenside_castling_rights(chess.BLACK):
        planes[:, :, 16] = 1.0

    return planes