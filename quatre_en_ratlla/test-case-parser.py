import numpy as np

WIDTH = 7
HEIGHT = 6

# Hay que guardar todos los datos ya procesados. Para cada entrada sacar los scores de cada movimiento
# para tener mas entradas, y simetria horizontal


def test():
    tests = []
    tl = 0
    with open("test/test-complete", "r") as f:
        tests = [l.strip().split(" ") for l in f.readlines()]
    scores = {}
    player = {-1: 0, 1: 0}
    for test in tests:
        tl += len(test[0])
        t, p = construirTablero(test[0])
        player[p] += 1
        s = int(test[1])
        if s not in scores.keys():
            scores[s] = 0
        scores[s] += 1


    print("SCORES")
    for key, value in sorted(scores.items()):
        print(f"{key} : {value/60:.4f}%")
    print("PLAYERS")
    for key, value in player.items():
        print(f"{key} : {value/60:.4f}%")
    print(tl)

def fillTestWoScore():
    c = 1
    tests = []
    sol = []
    with open("test/test-complete", "r") as f:
        fullTests = [l.strip().split(" ") for l in f.readlines()]
    for ft in fullTests:
        tests.append(ft[0])
        sol.append(ft[1])
    for t in range(len(tests)):
        print(c)
        for i in range(len(tests[t])):
            tests.append(tests[t][0:i+1])
        c += 1
    print(len(tests))
    print(tests[-1])
    with open("test/full-test-withouot-score","w") as f:
        f.writelines([f"{i}\n" for i in tests])

def removeDoubleTests():
    with open("test/full-test-without-score", "r") as f:
        fullTest = [l.strip() for l in f.readlines()]
    fullTest = list(set(fullTest))
    print(fullTest[fullTest.index("")])
    with open("test/full-no-dup", "w") as f:
        f.writelines("\n".join(sorted(fullTest)))

def applySimetry():
    with open("test/dataset-no-sim", "r") as f:
        fullTest = [l.rstrip().split() for l in f.readlines()]
    print(len(fullTest))
    for i in range(len(fullTest)):
        if len(fullTest[i]) == 1: 
            fullTest[i] = ["", fullTest[i][0]]
        p, s = fullTest[i][0], fullTest[i][1]
        if p != p[::-1]:
            fullTest.append([p[::-1], s])
    print(len(fullTest))
    with open("test/full-no-dup", "w") as f:
        f.writelines("\n".join(sorted([f"{t[0]} {t[1]}" for t in fullTest])))

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
    fdata = [(construirTablero(l[0]), int(l[1])) for l in data]
    return fdata

def func(x):
    return str(x+2)

data = loadData("connect-4/test/dataset")

print(len(data))

for i in range(len(data)):
    d,s = data[i]
    dd = np.fliplr(d)
    if not np.array_equal(dd,d):
        data.append((dd.flatten(), s))
    data[i] = (d.flatten(),s)

print(len(data))

with open("connect-4/test/simetria", "w") as f:
    f.writelines("\n".join([f"{''.join(map(func,t[0].tolist()))} {t[1]}" for t in data]))