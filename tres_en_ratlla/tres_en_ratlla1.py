import numpy as np
import random
def estimate_spectral_norm(A, n_iter=30, seed=0):
    """
    Estima ||A||_2 amb power iteration sobre A^T A.
    Retorna una aproximació de la norma espectral (2-norm).
    """
    rng = np.random.default_rng(seed)
    p = A.shape[1]
    x = rng.standard_normal(p)
    x /= np.linalg.norm(x) + 1e-30

    for _ in range(n_iter):
        y = A @ x
        z = A.T @ y          # z = (A^T A) x
        nz = np.linalg.norm(z)
        if nz == 0:
            return 0.0
        x = z / nz

    # Rayleigh quotient per aproximar el major autovalor de A^T A
    y = A @ x
    sigma = np.linalg.norm(y)  # sqrt(x^T A^T A x) = ||A x||
    return float(sigma)

def landweber(A, s, v0=None, omega=None,
              tol_res=1e-6, tol_step=1e-10,
              max_iter=10_000,
              delta=None, tau=1.05,
              norm_iters=30,
              history=True):
    """
    Minimitza ||A v - s||_2^2 amb Landweber:
        v_{k+1} = v_k + omega * A^T (s - A v_k)

    Params:
      A: (m, p)
      s: (m,)
      v0: inicial (p,), si None -> zeros
      omega: pas. Si None, tria omega = 1 / ||A||_2^2 (estimada)
      tol_res: tolerància del residual relatiu ||r||/||s||
      tol_step: tolerància del pas relatiu ||dv||/(||v||+eps)
      max_iter: iteracions màximes
      delta: si coneixes nivell de soroll (norma), activa discrepància: ||r|| <= tau*delta
      tau: factor del criteri de discrepància (1.01..1.2)
      norm_iters: iteracions per estimar ||A||_2
      history: si True retorna historial

    Retorna:
      v, info
    """
    A = np.asarray(A, dtype=float)
    s = np.asarray(s, dtype=float).reshape(-1)

    m, p = A.shape
    if s.shape[0] != m:
        raise ValueError(f"s ha de tindre longitud {m}, però té {s.shape[0]}")

    if v0 is None:
        v = np.zeros(p, dtype=float)
    else:
        v = np.asarray(v0, dtype=float).reshape(-1)
        if v.shape[0] != p:
            raise ValueError(f"v0 ha de tindre longitud {p}, però té {v.shape[0]}")

    # tria omega estable si no s'ha donat
    if omega is None:
        normA = estimate_spectral_norm(A, n_iter=norm_iters)
        if normA == 0.0:
            # A = 0: qualsevol v dona Av=0; millor retorn v0
            info = {"status": "A_zero", "iters": 0, "omega": 0.0}
            return v, info
        omega = 1.0 / (normA * normA)

    eps = 1e-30
    s_norm = np.linalg.norm(s) + eps

    res_hist = []
    step_hist = []

    status = "max_iter"
    it_used = 0

    for k in range(1, max_iter + 1):
        r = s - A @ v
        r_norm = np.linalg.norm(r)

        # criteri de discrepància (si hi ha soroll conegut)
        if delta is not None and r_norm <= tau * float(delta):
            status = "discrepancy"
            it_used = k - 1
            break

        # gradient i update
        g = A.T @ r
        dv = omega * g
        v_new = v + dv

        # criteri de parada per residual relatiu
        r_rel = r_norm / s_norm
        if r_rel <= tol_res:
            v = v_new
            status = "residual"
            it_used = k
            break

        # criteri de parada per canvi relatiu en v
        step_rel = np.linalg.norm(dv) / (np.linalg.norm(v_new) + eps)
        if step_rel <= tol_step:
            v = v_new
            status = "step"
            it_used = k
            break

        v = v_new
        it_used = k

        if history:
            res_hist.append(r_rel)
            step_hist.append(step_rel)

    info = {
        "status": status,
        "iters": it_used,
        "omega": float(omega),
    }
    if history:
        info["residual_rel_history"] = np.array(res_hist, dtype=float)
        info["step_rel_history"] = np.array(step_hist, dtype=float)

    return v, info

def sigmoid(z):
    # estable numèricament
    z = np.clip(z, -50, 50)
    return 1.0 / (1.0 + np.exp(-z))

def landweber_logistic(A, y, v0=None, eta=None,
                       l2=0.0,                 # regularització L2 (recomanat per al quadràtic)
                       tol_loss=1e-7,
                       tol_step=1e-10,
                       max_iter=50_000,
                       norm_iters=30,
                       history=True,
                       eps=1e-12):
    """
    Minimitza la cross-entropy logística:
        L(v) = - sum_i [ y_i log p_i + (1-y_i) log(1-p_i) ] + (l2/2)||v||^2
        on p = sigmoid(A v)

    Update (estil Landweber):
        v <- v + eta * A^T (y - p) - eta*l2*v

    eta:
      Si eta és None, es tria un pas estable aproximat amb ||A||_2:
        gradient Lipschitz <= (1/4)||A||^2 + l2
        eta <= 1 / ( (1/4)||A||^2 + l2 )
    """
    A = np.asarray(A, dtype=float)
    y = np.asarray(y, dtype=float).reshape(-1)

    m, p = A.shape
    if y.shape[0] != m:
        raise ValueError(f"y ha de tindre longitud {m}, però té {y.shape[0]}")

    if v0 is None:
        v = np.zeros(p, dtype=float)
    else:
        v = np.asarray(v0, dtype=float).reshape(-1)
        if v.shape[0] != p:
            raise ValueError(f"v0 ha de tindre longitud {p}, però té {v.shape[0]}")

    # tria pas estable si no s'ha donat
    if eta is None:
        # reutilitza la teua funció estimate_spectral_norm si ja la tens definida
        normA = estimate_spectral_norm(A, n_iter=norm_iters)
        L = 0.25 * (normA * normA) + float(l2)
        if L == 0.0:
            info = {"status": "A_zero", "iters": 0, "eta": 0.0}
            return v, info
        eta = 1.0 / L

    loss_hist = []
    step_hist = []

    status = "max_iter"
    it_used = 0

    def logistic_loss(p_hat):
        # cross-entropy estable
        p_hat = np.clip(p_hat, eps, 1 - eps)
        ce = -(y * np.log(p_hat) + (1 - y) * np.log(1 - p_hat)).mean()
        reg = 0.5 * l2 * np.dot(v, v)
        return float(ce + reg)

    prev_loss = None

    for k in range(1, max_iter + 1):
        z = A @ v
        p_hat = sigmoid(z)

        # gradient de la loss: A^T (p - y) + l2 v
        # update amb signe "Landweber": v <- v + eta * A^T (y - p) - eta*l2*v
        r = (y - p_hat)                 # residual logístic
        g = A.T @ r - l2 * v
        dv = eta * g
        v_new = v + dv

        step_rel = np.linalg.norm(dv) / (np.linalg.norm(v_new) + 1e-30)

        # loss (opcional, per criteri de parada)
        if history or tol_loss is not None:
            v_old = v
            v = v_new
            curr_loss = logistic_loss(sigmoid(A @ v))
            v = v_old
        else:
            curr_loss = None

        # criteri per pas
        if step_rel <= tol_step:
            v = v_new
            status = "step"
            it_used = k
            break

        # criteri per millora de loss
        if prev_loss is not None and tol_loss is not None:
            if abs(prev_loss - curr_loss) <= tol_loss * (abs(prev_loss) + 1.0):
                v = v_new
                status = "loss"
                it_used = k
                break

        v = v_new
        prev_loss = curr_loss
        it_used = k

        if history:
            loss_hist.append(curr_loss)
            step_hist.append(step_rel)

    info = {"status": status, "iters": it_used, "eta": float(eta), "l2": float(l2)}
    if history:
        info["loss_history"] = np.array(loss_hist, dtype=float)
        info["step_rel_history"] = np.array(step_hist, dtype=float)

    return v, info


def ia(bit,posicio,vector):
    if bit=="0":
        return aleatori(posicio)
    elif bit in ["1","2"]:
        candidats=moviments(posicio)
        if bit=="1":
            valoracions=list(map(lambda x: lineal(x,vector),candidats))
        elif bit=="2":
            valoracions=list(map(lambda x: quadratic(x,vector),candidats))
        else:
            print("ERROR. Algoritme no trobat.")
        valoracions=list(map(lambda x: round(x,10),valoracions))
        seleccionats=[]
        maxim=max(valoracions)
        for i in range(len(candidats)):
            if valoracions[i]==maxim:
                seleccionats.append(candidats[i])
        return random.choice(seleccionats)
    else:
        print("Model inexistent.")
        
def guanyar(posicio):
    posibilitats=[[0,1,2],[3,4,5],[6,7,8],[0,3,6],[1,4,7],[2,5,8],[0,4,8],[2,4,6]]
    moviments=False
    for i in posibilitats:
        candidat=[posicio[i[0]],posicio[i[1]],posicio[i[2]]]
        if candidat==[1,1,1]:
            return 1
        elif candidat==[-1,-1,-1]:
            return -1
        elif 0 in candidat:
            moviments=True
    if moviments:
        return ""
    else:
        return 0  
    
def aleatori(posicio):
    cond=False
    while not cond:
        al=random.choice(list(range(9)))
        if posicio[al]==0:
            posicio[al]=1
            cond=True
    return posicio  
    
def lineal(raw,vector):
    posicio=raw2ia("1",raw)
    valor=0
    for i in range(18):
        valor=valor+posicio[i]*vector[i]
    valor=1/(1+np.exp(-valor))
    return float(valor)

def raw2ia(bit,raw):
    posicio=[]
    if bit=="1":
        for i in raw:
            if i==1:
                posicio.append(1)
            else:
                posicio.append(0)
        for i in raw:
            if i==-1:
                posicio.append(1)
            else:
                posicio.append(0)
        return posicio
    elif bit=="2":
        for i in raw:
            if i==1:
                posicio.append(1)
            else:
                posicio.append(0)
        for i in raw:
            if i==-1:
                posicio.append(1)
            else:
                posicio.append(0)
        for i in range(18):
            for j in range(i,18):
                posicio.append(posicio[i]*posicio[j])
        return posicio
                
def moviments(posicio):
    resultat=[]
    for i in range(9):
        if posicio[i]==0:
            aux=posicio[:]
            aux[i]=1
            resultat.append(aux)
    return resultat 

def jugar_partides(bit,vector):
    posicions={}
    for i in range(10000):
        posicio=[0,0,0,0,0,0,0,0,0]
        victoria=""
        partida=[]
        torn=1
        while victoria not in [1,-1,0]:
            posicio=ia(bit,posicio,vector)
            partida.append([tuple(posicio),torn])
            torn=-torn
            for i in range(9):
                posicio[i]=-posicio[i]
            victoria=guanyar(posicio)
        guanyador=-torn
        for j in partida:
            if j[0] not in posicions.keys():
                posicions[j[0]]=[0,0]
            if victoria==0:
                posicions[j[0]][0]+=1
                posicions[j[0]][1]+=1
            elif guanyador==j[1]:
                posicions[j[0]][0]+=1
            else:
                posicions[j[0]][1]+=1
    for i in posicions.keys()             :
        posicions[i]=posicions[i][0]/sum(posicions[i])
    return posicions

def entrenar(bit,vector):
    posicions=jugar_partides(bit,vector)
    A=[]
    b=[]
    for i in posicions.keys():
        A.append(raw2ia(bit,i))
        b.append(posicions[i])
    vector,info=landweber_logistic(A,b,l2=0.1)
    vector=list(vector)
    for i in range(len(vector)):
        vector[i]=float(vector[i])
    return vector

def batalla(bit1,bit2,vector1,vector2):
    cont=[0,0,0]
    for k in range(1000):
        posicio=[0,0,0,0,0,0,0,0,0]
        torn=random.choice([1,-1])
        victoria=""
        while victoria not in [1,0,-1]:
            if torn==1:
                posicio=ia(bit1,posicio,vector1)
                torn=-1
            else:
                aux=[]
                for i in posicio:
                    aux.append(-i)
                aux=ia(bit2,aux,vector2)
                for i in range(9):
                    posicio[i]=-aux[i]
                torn=1
            victoria=guanyar(posicio)
        if victoria==1:
            cont[0]+=1
        elif victoria==-1:
            cont[1]+=1 
        else:
            cont[2]+=1
    return cont

def quadratic(raw,vector):
    posicio=raw2ia("2",raw)
    valor=0
    for i in range(189):
        valor=valor+posicio[i]*vector[i]
    valor=1/(1+np.exp(-valor))
    return float(valor)

def jugar(bit,vector):
    posicio=[0,0,0,0,0,0,0,0,0]
    torn=random.choice([1,-1])
    victoria=""
    while victoria not in [1,0,-1]:
        if torn==1:
            imprimir(posicio)
            mov=moure(posicio)
            posicio[mov]=1
            torn=-1
        else:
            aux=[]
            for i in posicio:
                aux.append(-i)
            aux=ia(bit,aux,vector)
            for i in range(len(aux)):
                posicio[i]=-aux[i]
            torn=1
        victoria=guanyar(posicio)
    imprimir(posicio)
    if victoria==1:
        print("Has guanyaaaat!!!")
    elif victoria==-1:
        print("Ho sent. Has perdut :(")
    else:
        print("Empat.")  

def moure(posicio):
    mov=""
    valid=False
    while not valid:
        mov=input("Casella: ")
        try:
            mov=int(mov)
            if mov in [0,1,2,3,4,5,6,7,8]:
                if posicio[mov]==0:
                    valid=True
                else:
                    print("ERROR. Jugada il·legal.")
            else:
                print("ERROR. Casella inexistent.")
        except:
            print("ERROR. Casella no numèrica.")
    return mov
def imprimir(posicio):
    cont=0
    for i in posicio:
        cont=cont+1
        if i==1:
            print("X",end=" ")
        elif i==-1:
            print("O",end=" ")
        else:
            print("-",end=" ")
        if cont%3==0:
            print()
    print()
vector1=[]
vector2=[]
vector=[]
for i in range(18):
    vector1.append(0)
for i in range(189):
    vector2.append(0)
resp=""
bit="1"
while resp!="0":
    print("\n\n\n###   Tres en Ratlla   ###\n")
    print("Opcions:")
    print("0-Eixir")
    print("1-Seleccionar model")
    print("2-Jugar")
    print("3-Entrenar")
    print("4-Lluita de models")
    print()
    resp=input("Opció: ")
    if resp=="1":
        bit=""
        while bit not in ["0","1","2"]:
            bit=input("Bit: ")
        if bit=="1":
            vector=vector1
        elif bit=="2":
            vector=vector2
        elif bit=="0":
            vector=[]
    elif resp=="2":
        jugar(bit,vector)
    elif resp=="3":
        vector=entrenar(bit,vector)
        if bit=="1":
            vector1=vector
        elif bit=="2":
            vector2=vector
        print(vector)
    elif resp=="4":
        print(batalla(bit,"0",vector,[]))
    elif resp=="0":
        pass
    else:
        print("ERROR. Resposta incorrecta.")