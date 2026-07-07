import numpy as np
import game as game

WIDTH = 7
HEIGHT = 6

# Internas
LAYERS = [64, 64, 32]

VALIDATION = 0.2
TEST = 0.1

BATCH = 10
EPOCHS = 30


# En model.py
def load_data(filepath: str):
    with open(filepath, "r") as f:
        data = [l.rstrip().split() for l in f.readlines()]

    x = np.array([
        [int(c) - 2 for c in board]
        for board, _ in data
    ])

    y = np.array([
        int(value)
        for _, value in data
    ])
    return x, y

if __name__=="__main__":
    connect = game.Connect4Game("1172364512324")
    print(connect, "\n")

    for i in range(WIDTH):
        print(connect.pred_move(i), "\n")