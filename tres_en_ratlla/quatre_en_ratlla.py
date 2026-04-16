import numpy as np
import random

#norma espectral aproximada per a ajustar el pas d'aprenentatge

def estimate_spectral_norm(A, n_iter=30, seed=0):
    rng = np.random.default_rng(seed)
    p = A.shape[1]
    x = rng.standard_normal(p)
    x /= np.linalg.norm(x) + 1e-30
    for _ in range(n_iter):
        y = A @ x
        z = A.T @ y
        nz = np.linalg.norm(z)
        if nz == 0: return 0.0
        x = z / nz
    return float(np.linalg.norm(A @ x))

# Algoritmo de Landweber para regresión logística con regularización L2

def landweber_logistic(A, y, v0=None, eta=None, l2=0.01, tol_loss=1e-7, tol_step=1e-10, max_iter=10_000, norm_iters=30):
    A = np.asarray(A, dtype=float)
    y = np.asarray(y, dtype=float).reshape(-1)
    m, p = A.shape
    if v0 is None: v = np.zeros(p, dtype=float)
    else: v = np.asarray(v0, dtype=float).reshape(-1)

    if eta is None:
        normA = estimate_spectral_norm(A, n_iter=norm_iters)
        L = 0.25 * (normA * normA) + float(l2)
        eta = 1.0 / L if L > 0 else 0.1

    for k in range(1, max_iter + 1):
        z = A @ v
        p_hat = 1.0 / (1.0 + np.exp(-np.clip(z, -50, 50)))
        r = (y - p_hat)
        g = A.T @ r - l2 * v
        dv = eta * g
        v_new = v + dv
        if np.linalg.norm(dv) / (np.linalg.norm(v_new) + 1e-30) <= tol_step:
            v = v_new
            break
        v = v_new
    return v, {"iters": k}


#sigmoide de tota la vida

def sigmoid(z):
    z = np.clip(z, -50, 50)
    return 1.0 / (1.0 + np.exp(-z))

# Función para obtener las 8 simetrías de un tablero 4x4

def obtener_simetrias(posicio_vector):
    
    tauler = np.array(posicio_vector).reshape(4, 4)
    simetries = set()
    
    for i in range(4):
        # Rotaciones de 0, 90, 180, 270 grados
        rotat = np.rot90(tauler, i)
        simetries.add(tuple(rotat.flatten()))
        
        simetries.add(tuple(np.fliplr(rotat).flatten()))
        
    return [list(s) for s in simetries]

# --- LÓGICA DEL JUEGO 4X4 ---

def guanyar(posicio):
    # 10 combinaciones para ganar en un 4x4
    posibilitats = [
        [0,1,2,3], [4,5,6,7], [8,9,10,11], [12,13,14,15], # Horizontales
        [0,4,8,12], [1,5,9,13], [2,6,10,14], [3,7,11,15], # Verticales
        [0,5,10,15], [3,6,9,12]                          # Diagonales
    ]
    hi_ha_buits = False
    for i in posibilitats:
        c = [posicio[i[0]], posicio[i[1]], posicio[i[2]], posicio[i[3]]]
        if c == [1,1,1,1]: return 1
        if c == [-1,-1,-1,-1]: return -1
        if 0 in c: hi_ha_buits = True
    return "" if hi_ha_buits else 0

def raw2ia(bit, raw):
    # Lineal: 16 mías + 16 rival = 32 features
    pos = [1 if x == 1 else 0 for x in raw] + [1 if x == -1 else 0 for x in raw]
    if bit == "1": return pos
    # Cuadrático: 32 lineales + 528 interacciones = 560 features
    res = pos[:]
    for i in range(32):
        for j in range(i, 32):
            res.append(pos[i] * pos[j])
    return res

def ia(bit, posicio, vector):
    if bit == "0":
        idx = [i for i, x in enumerate(posicio) if x == 0]
        if not idx: return posicio
        res = posicio[:]; res[random.choice(idx)] = 1
        return res
    
    candidats = []
    for i in range(16):
        if posicio[i] == 0:
            aux = posicio[:]; aux[i] = 1; candidats.append(aux)
    
    if not candidats: return posicio
    
    # Evaluación según el modelo
    if bit == "1":
        vals = [sigmoid(np.dot(raw2ia("1", c), vector)) for c in candidats]
    else:
        vals = [sigmoid(np.dot(raw2ia("2", c), vector)) for c in candidats]
    
    maxim = round(max(vals), 10)
    best = [candidats[i] for i, v in enumerate(vals) if round(v, 10) == maxim]
    return random.choice(best)

def jugar_partides(bit, vector, n=1000):
    pos_stats = {}
    for _ in range(n):
        pos = [0]*16; partida = []; torn = 1; vic = ""
        while vic not in [1, -1, 0]:
            pos = ia(bit, pos, vector)
            partida.append((tuple(pos), torn))
            torn = -torn
            pos = [-x for x in pos]
            vic = guanyar(pos)
        
        guanyador = -torn
        for p, t in partida:
            if p not in pos_stats: pos_stats[p] = [0, 0]
            if vic == 0: pos_stats[p][1] += 1 # Empate cuenta como no-victoria
            elif guanyador == t: pos_stats[p][0] += 1
            else: pos_stats[p][1] += 1
    return {k: v[0]/sum(v) for k, v in pos_stats.items()}

def entrenar(bit, vector):
    print("Jugant partides per a recol·lectar dades...")
    dades = jugar_partides(bit, vector)
    
    A_aug, b_aug = [], []
    print(f"Aplicant simetries a {len(dades)} estats únics...")
    for pos, prob in dades.items():
        
        for s in obtener_simetrias(list(pos)):
            A_aug.append(raw2ia(bit, s))
            b_aug.append(prob)
            
    print(f"Entrenant model amb {len(A_aug)} mostres...")
    new_v, info = landweber_logistic(A_aug, b_aug, l2=0.01)
    return list(new_v)

# --- INTERFAZ DE USUARIO ---

def imprimir(p):
    for i in range(0, 16, 4):
        print(" ".join(["X" if x==1 else ("O" if x==-1 else "-") for x in p[i:i+4]]))
    print()

def jugar(bit, vector):
    if not vector: return print("Selecciona model primer.")
    p = [0]*16; torn = random.choice([1, -1]); vic = ""
    while vic not in [1, 0, -1]:
        if torn == 1:
            imprimir(p)
            m = int(input("Casella (0-15): "))
            if p[m] == 0: p[m] = 1; torn = -1
        else:
            aux = ia(bit, [-x for x in p], vector)
            p = [-x for x in aux]; torn = 1
        vic = guanyar(p)
    imprimir(p)
    print("Guanyador: ", "Tu" if vic==1 else ("IA" if vic==-1 else "Empat"))

# --- BUCLE PRINCIPAL ---
v1, v2 = [0.0]*32, [0.0]*560
vector, bit = v1, "1"

while True:
    print("\n--- TRES EN RATLLA 4x4 (Simetries Actives) ---")
    print("1-Seleccionar Model | 2-Jugar | 3-Entrenar | 0-Eixir")
    op = input("Opció: ")
    if op == "1":
        bit = input("Bit (1:Lineal, 2:Cuadràtic): ")
        vector = v1 if bit == "1" else v2
    elif op == "2": 
        print("Juguem! Tu comences. Indica la casella (0-15) on vols posar el teu 'X'.")
        jugar(bit, vector)
    elif op == "3":
        vector = entrenar(bit, vector)
        if bit == "1": v1 = vector
        else: v2 = vector
    elif op == "0": break