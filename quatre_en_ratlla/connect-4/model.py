import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split

WIDTH = 7
HEIGHT = 6

# Internas
LAYERS = [64, 64, 32]

VALIDATION = 0.2
TEST = 0.1

BATCH = 10
EPOCHS = 30

# Crea el tablero para cada entrada de test
# Ya no hace falta
def construirTablero(smov:str):
    tablero = np.array([[0 for i in range(WIDTH)] for i2 in range(HEIGHT)])
    mov = [int(c) for c in smov]
    l = len(mov)
    player = 1 if l%2==0 else -1
    for i in range(l):
        n = mov[i]-1
        hacerTurno(tablero, n, (1 if i%2==0 else -1)*player)
    s = sum(tablero[:,0])
    return tablero

# Pone una pieza en la columna n para el jugador p
def hacerTurno(t, n, p):
    s = sum(abs(t[:,n]))
    t[HEIGHT-1-s,n] = p

def check_victory(board, col):
    rows, cols = board.shape
    col = col-1
    
    # 1. Encontrar la fila donde cae la ficha
    for row in range(rows-1, -1, -1):
        if board[row][col] == 0:
            break
    else:
        return False  # columna llena
    
    # 2. Colocar ficha temporalmente (siempre 1)
    board[row][col] = 1

    def count_dir(dx, dy):
        count = 1
        
        # hacia un lado
        r, c = row + dy, col + dx
        while 0 <= r < rows and 0 <= c < cols and board[r][c] == 1:
            count += 1
            r += dy
            c += dx
        
        # hacia el otro
        r, c = row - dy, col - dx
        while 0 <= r < rows and 0 <= c < cols and board[r][c] == 1:
            count += 1
            r -= dy
            c -= dx
        
        return count

    # 3. Comprobar direcciones
    victory = (
        count_dir(1, 0) >= 4 or   # horizontal
        count_dir(0, 1) >= 4 or   # vertical
        count_dir(1, 1) >= 4 or   # diagonal ↘
        count_dir(1, -1) >= 4     # diagonal ↗
    )

    # 4. Deshacer jugada
    board[row][col] = 0
    return victory

def loadData(filepath: str):
    with open(filepath, "r") as f:
        data = [l.rstrip().split() for l in f.readlines()]
    fdata = [([int(c)-2 for c in l[0]], int(l[1])) for l in data]
    return fdata

if __name__=="__main__":
    data = loadData("connec t-4/test/dataset")
    print(len(data))
    print(data[13])
    x = np.array([t[0] for t in data])
    # Dividir entre 21 para normalizar a -1, 1
    y = np.array([t[1] for t in data])
    print(len(x), len(y))

    x_train, x_test, y_train, y_test = train_test_split(x, y, test_size=TEST)
    # Crear modelo
    print(x_train[2], y_train[2])
    model = tf.keras.models.Sequential()
    model.add(tf.keras.Input(shape=(WIDTH*HEIGHT,)))
    for l in LAYERS:
        model.add(tf.keras.layers.Dense(l, activation="tanh"))
    model.add(tf.keras.layers.Dense(1, activation="linear"))

    callback = tf.keras.callbacks.EarlyStopping(
        monitor="val_loss",
        patience=5,
        restore_best_weights=True
    )

    ## Compilar modelo
    model.compile(optimizer="adam", loss="mse", metrics=["mae"])

    ## Entrenar modelo
    model.fit(x_train, y_train, batch_size=BATCH, epochs=EPOCHS, validation_split=VALIDATION, callbacks=[callback])

    ## Evaluar modelo
    model.evaluate(x_test, y_test)

    dato = construirTablero("11").flatten().reshape((1,WIDTH*HEIGHT))
    print(dato)

    print(model.predict(dato))