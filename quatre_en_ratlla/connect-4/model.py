import random
import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split

WIDTH = 7
HEIGHT = 6
# Internas
LAYERS = [64, 128]

VALIDATION = 0.2
TEST = 0.1
BATCH = 10
EPOCHS = 5

# Crea el tablero para cada entrada de test
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

def loadData(filepath: str):
    with open(filepath, "r") as f:
        data = [l.rstrip().split() for l in f.readlines()]
    fdata = [(construirTablero(l[0]).flatten(), int(l[1])) for l in data]
    return fdata

t = construirTablero("1")
print(t)

data = loadData("test/dataset")
print(len(data))
x = np.array([t[0] for t in data])
# Dividir entre 27 para normalizar a -1, 1
y = np.array([t[1]/21 for t in data])
print(len(x), len(y))

x_train, x_test, y_train, y_test = train_test_split(x, y, test_size=TEST)
# Crear modelo

model = tf.keras.models.Sequential()
model.add(tf.keras.Input(shape=(WIDTH*HEIGHT,)))
for l in LAYERS:
    model.add(tf.keras.layers.Dense(l, activation="tanh"))
model.add(tf.keras.layers.Dense(1, activation="tanh"))

# Compilar modelo
model.compile(optimizer="adam", loss="mse", metrics=["mae"])

# Entrenar modelo
model.fit(x_train, y_train, batch_size=BATCH, epochs=EPOCHS, validation_split=VALIDATION)

# Evaluar modelo
model.evaluate(x_test, y_test)