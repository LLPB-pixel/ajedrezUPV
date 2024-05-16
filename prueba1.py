import chess
import random

def main():
    board = chess.Board()
    print(board)
    while True:
        side = input("Dime si quieres negras (n) o blancas (b)")
        if side.upper() == "B":
            jugada = input("haz tu primera jugada")
            break
        elif side.upper() == "N":
            break
    jugada = jugadalegal(board, jugada, side)
    board.push(chess.Move.from_uci(jugada))

    while not board.is_game_over():
        jugadabot(board, side)
        jugadapesona(board, side)
    if board.is_stalemate():
        print("Rey ahogado")
    if board.is_checkmate():
        print("jaque mate!")

    return

def get_legal_moves(board, color):
    legal_moves = []
    for move in board.legal_moves:
        if board.piece_at(move.from_square).color == color:
            legal_moves.append(move)
    return legal_moves


def jugadalegal(board, jugada, color):
    if color.upper() == "B":
        legal_moves = get_legal_moves(board, chess.WHITE)
    else:
        legal_moves = get_legal_moves(board, chess.BLACK)

    for move in legal_moves:
        if str(move) == jugada:
            return jugada


    else:
        jug = input("Jugada incorrecta, vuelve a escribirla: ")
        return jugadalegal(board, jug, color)

def jugadabot(board, color):
    if color.upper() == "B":
        legal_moves = get_legal_moves(board, chess.BLACK)
    else:
        legal_moves = get_legal_moves(board, chess.WHITE)

    random_move = random.choice(legal_moves)
    board.push(random_move)
    print("Jugada realizada por el bot:", random_move)
    print(board)
    return

def jugadapesona(board, color):
    jug = input("dime qué jugada quieres hacer")
    jug = jugadalegal(board, jug, color)
    board.push(chess.Move.from_uci(jug))
    return

def deshacer(board):
    board.pop()
    board.pop()
    return 

def valores(pieza):
    if pieza == chess.PAWN:
        return 1
    if pieza == chess.KNIGHT or pieza == chess.BISHOP:
        return 3
    if pieza == chess.ROOK:
        return 5
    if pieza == chess.QUEEN:
        return 9
    else:
        return 0

main()


