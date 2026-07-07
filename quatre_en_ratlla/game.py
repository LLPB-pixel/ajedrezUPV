import numpy as np

class Connect4Game:

    WIDTH = 7
    HEIGHT = 6

    def __init__(self, moves = "") -> None:
        self._board = np.array([[0 for _ in range(self.WIDTH)] for _ in range(self.HEIGHT)])
        if moves != "":
            self.build_board(moves)

    def __str__(self) -> str:
        str_board = []
        for i in range(self.HEIGHT):
            row = []
            for p in self._board[i,:]:
                c = ""
                match p:
                    case 0: c = "_"
                    case 1: c = "x"
                    case -1: c = "o"
                row.append(f"{c}")
            str_board.append((" ".join(row)))
        return "\n".join(str_board)

    def build_board(self, moves):
        moves = [int(c) for c in moves]
        l = len(moves)
        player = 1 if l%2==0 else -1
        for i in range(l):
            column = moves[i]-1
            self.make_move(column, (1 if i%2==0 else -1)*player)

    def make_move(self, column, player):
        s = sum(abs(self._board[:,column]))
        self._board[self.HEIGHT-1-s, column] = player

    def set_board(self, board):
        self._board = board

    def pred_move(self, column): 
        new_game = Connect4Game()
        new_game.set_board(self._board*(-1))
        new_game.make_move(column, -1)
        return new_game

    def check_victory(self, column):
        rows, columns = self._board.shape
        column= column-1
    
        s = sum(abs(self._board[:,column]))
        if s == self.HEIGHT:
            return False  # columna llena
        
        row = self.HEIGHT-1-s
    
        # 2. colocar ficha temporalmente (siempre 1)
        self._board[row][column] = 1

        def count_dir(dx, dy):
            count = 1
        
            # hacia un lado
            r, c = row + dy, column + dx
            while 0 <= r < rows and 0 <= c < columns and self._board[r][c] == 1:
                count += 1
                r += dy
                c += dx
        
            # hacia el otro
            r, c = row - dy, column - dx
            while 0 <= r < rows and 0 <= c < columns and self._board[r][c] == 1:
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
        self._board[row][column] = 0
        return victory
