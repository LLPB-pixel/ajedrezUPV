import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
import matplotlib.pyplot as plt
from collections import defaultdict

WIDTH = 7
HEIGHT = 6

# Internas
LAYERS = [64, 64, 32]

VALIDATION = 0.2
TEST = 0.1

BATCH = 128   # antes 10 -> con 165k+ filas, batch pequeño hace el entrenamiento
              # innecesariamente lento y el gradiente más ruidoso
EPOCHS = 50   # subimos el techo; EarlyStopping + ReduceLROnPlateau deciden cuándo parar

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

def encode_2ch(board_flat):
    """Convierte un tablero plano (42,) en 2 canales binarios en vez de
    un único canal con valores {-1,0,1}. Esto evita que la CNN interprete
    una relación de orden falsa entre 'vacío' y 'ficha rival'."""
    b = np.asarray(board_flat).reshape(HEIGHT, WIDTH)
    ch1 = (b == 1).astype(np.float32)   # fichas "propias" (perspectiva del que mueve)
    ch2 = (b == -1).astype(np.float32)  # fichas rivales
    return np.stack([ch1, ch2], axis=-1)  # shape (H, W, 2)


if __name__=="__main__":
    data = loadData("connect_four/test/dataset")
    print(f"Total dataset size: {len(data)}")

    # x: tableros planos (N, 42), necesarios tal cual para el chequeo de duplicados
    x = np.array([t[0] for t in data])
    # Dividir entre 21 para normalizar a -1, 1
    y = np.array([t[1] for t in data]) / 21.0
    print(f"Features shape: {x.shape}, Labels shape: {y.shape}")
    print("Example raw row:", data[13])

    # Chequeo de duplicados: ¿hay tableros idénticos con resultados distintos?
    # (usamos x, no x_2ch, porque necesitamos una clave hasheable simple)
    board_to_labels = defaultdict(list)
    for xi, yi in zip(x, y):
        board_to_labels[tuple(xi)].append(yi)

    dupes = {k: v for k, v in board_to_labels.items() if len(v) > 1}
    if dupes:
        variances = [np.var(v) for v in dupes.values()]
        print(f"Boards duplicados: {len(dupes)} / {len(x)}")
        print(f"Varianza media de y en duplicados: {np.mean(variances):.4f}")
        print(f"Std media equivalente: {np.sqrt(np.mean(variances)):.4f}")
    else:
        print("No hay boards duplicados en el dataset.")

    # Encoding a 2 canales
    x_2ch = np.array([encode_2ch(xi) for xi in x])  # shape (N, H, W, 2)

    # Data augmentation: Connect-4 es simétrico especularmente (voltear
    # izquierda-derecha no cambia quién gana), así que duplicamos el
    # dataset gratis y forzamos invarianza especular
    x_flipped = x_2ch[:, :, ::-1, :]
    x_aug = np.concatenate([x_2ch, x_flipped], axis=0)
    y_aug = np.concatenate([y, y], axis=0)
    print(f"Dataset tras augmentation: {x_aug.shape[0]} muestras")

    x_train, x_test, y_train, y_test = train_test_split(
        x_aug, y_aug, test_size=TEST, random_state=42
    )

    # Crear modelo: CNN con más capacidad y receptive field más grande
    # (2 capas 4x4 + 1 capa 3x3 cubren mejor el tablero 6x7 que 2 capas
    # 3x3/4x4 sueltas), más BatchNorm para estabilizar el entrenamiento
    model = tf.keras.models.Sequential([
        tf.keras.Input(shape=(HEIGHT, WIDTH, 2)),
        tf.keras.layers.Conv2D(64, (4,4), padding="same"),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.Activation("relu"),
        tf.keras.layers.Conv2D(64, (4,4), padding="same"),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.Activation("relu"),
        tf.keras.layers.Conv2D(128, (3,3), padding="same"),
        tf.keras.layers.BatchNormalization(),
        tf.keras.layers.Activation("relu"),
        tf.keras.layers.Flatten(),  
        tf.keras.layers.Dense(128, activation="relu"),
        tf.keras.layers.Dropout(0.3),
        tf.keras.layers.Dense(1, activation="linear"),
    ])

    early_stop = tf.keras.callbacks.EarlyStopping(
        monitor="val_loss",
        patience=8,
        restore_best_weights=True
    )

    # El val_loss oscilaba con LR fijo; bajamos el LR cuando se estanca
    reduce_lr = tf.keras.callbacks.ReduceLROnPlateau(
        monitor="val_loss", factor=0.5, patience=3, min_lr=1e-6
    )

    ## Compilar modelo
    model.compile(optimizer="adam", loss="mse", metrics=["mae"])

    ## Entrenar modelo
    print("\n--- Training Model ---")
    history = model.fit(
        x_train, y_train,
        batch_size=BATCH,
        epochs=EPOCHS,
        validation_split=VALIDATION,
        callbacks=[early_stop, reduce_lr]
    )

    ## Evaluar modelo
    print("\n--- Evaluating Model ---")
    eval_results = model.evaluate(x_test, y_test, verbose=0)
    print(f"Test Loss (MSE): {eval_results[0]:.4f}")
    print(f"Test MAE: {eval_results[1]:.4f}")

    # 1. Predicciones y correlación
    preds_test = model.predict(x_test).flatten()
    std_preds = preds_test.std()
    std_real = y_test.std()
    corr = np.corrcoef(preds_test, y_test)[0,1]

    # Sign agreement: are both positive, both negative, or both zero?
    same_sign = np.sign(preds_test) == np.sign(y_test)
    accuracy_direction = np.mean(same_sign)

    print("\n--- Detailed Stats ---")
    print(f"Standard Deviation of Predictions: {std_preds:.4f}")
    print(f"Standard Deviation of Real values: {std_real:.4f}")
    print(f"Pearson Correlation Coefficient: {corr:.4f}")
    print(f"Directional Accuracy (same sign): {accuracy_direction * 100:.2f}%")

    print("\n--- Top 10 Predictions vs Real ---")
    for i, (p, r) in enumerate(zip(preds_test[:10], y_test[:10])):
        print(f"Sample {i+1:2d}: Pred = {p:6.4f} | Real = {r:6.4f} | Diff = {abs(p-r):6.4f}")

    # 1b. Diagnóstico: peores predicciones, para ver si se concentran en
    # tableros con pocas fichas (early game) o muchas (end game)
    print("\n--- Worst 15 Predictions ---")
    errors = np.abs(preds_test - y_test)
    worst_idx = np.argsort(errors)[-15:][::-1]
    for i in worst_idx:
        n_pieces = int(np.sum(x_test[i]))  # suma de los 2 canales binarios
        print(f"fichas: {n_pieces:2d}  pred: {preds_test[i]:6.4f}  real: {y_test[i]:6.4f}  diff: {errors[i]:.4f}")

    # 2. Prueba con una posición donde el resultado NO deba ser neutral
    print("\n--- Custom Position Test ---")
    # Tablero con movimientos '1122334'. OJO: esta secuencia ya deja
    # "O O O O" en la fila inferior -> victoria ya consumada para O,
    # no para X (el comentario original estaba equivocado).
    tablero_test = construirTablero("1122334")
    print("Board state for moves '1122334':")
    for row in tablero_test:
        row_str = " ".join(["X" if cell == 1 else "O" if cell == -1 else "." for cell in row])
        print(f"  {row_str}")

    dato2 = encode_2ch(tablero_test.flatten()).reshape((1, HEIGHT, WIDTH, 2))
    pred_custom = model.predict(dato2)[0][0]
    print(f"Model prediction for custom position: {pred_custom:.4f} "
          f"(O ya tiene 4 en raya en la fila inferior, se espera un valor fuertemente negativo)")

    # 3. Plots
    print("\n--- Generating Plots ---")

    # Plot 1: Training curves
    plt.figure(figsize=(12, 5))

    plt.subplot(1, 2, 1)
    plt.plot(history.history["loss"], label="Train Loss (MSE)", color="blue", lw=2)
    plt.plot(history.history["val_loss"], label="Validation Loss", color="orange", lw=2)
    plt.title("Model Training Loss")
    plt.xlabel("Epoch")
    plt.ylabel("Loss (MSE)")
    plt.legend()
    plt.grid(True, linestyle="--", alpha=0.6)

    # Plot 2: Scatter predictions vs real
    plt.subplot(1, 2, 2)
    plt.scatter(y_test, preds_test, alpha=0.3, color="purple")
    # Línea ideal y = x
    ideal_x = np.linspace(y_test.min(), y_test.max(), 100)
    plt.plot(ideal_x, ideal_x, color="red", linestyle="--", label="Ideal (y = x)")
    plt.title("Predictions vs Real Labels")
    plt.xlabel("Real Normalized Score")
    plt.ylabel("Predicted Score")
    plt.legend()
    plt.grid(True, linestyle="--", alpha=0.6)

    plt.tight_layout()
    plot_path = "connect_four/test_results_plots.png"
    plt.savefig(plot_path)
    print(f"Saved summary plots to: {plot_path}")

    plt.show()