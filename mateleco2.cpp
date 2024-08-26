#include <iostream>
#include <string>
#include <cmath>
#include <tuple>

using namespace std;

class TableroAjedrez {

public:
    //atributos esenciales
    char tablero[8][8];
    bool white_turn = true;

    //atributos secundarios
    string registroJugadas[400]; //estos dos atributos estan por si acos queremos generar una funcion que deshaga jugadas
    int numJugada = 0; // para el registro de jugadas


    // Regla triple repeticion
    string registroPosiciones[101][2]; // esta estructura cuenta las veces que se ha repetido una posicion
    int numJugadaTFR = 0; //este numero sirve como indice de la estrucutra de datos anterior

    //Regla 50 jugadas
    int regla50jugadas = 0;


    //comprobar si se ha acabdola partida y pq
    bool gameOverByTFR = false;
    bool gameOverBy50MoveRule = false;
    bool gameOver = false;
    bool gameOverByStalemate = false;
    bool gameOverByCheckMate = false;


    //tema enroque y tal
    bool whiteKingMoved = false;
    bool blackKingMoved = false;
    bool whiteRookMoved[2] = {false, false}; // izquierda y derecha, mejor que añadir una variable más a cada pieza
    bool blackRookMoved[2] = {false, false};


    //Coordenadas de los reyes
    int cordReyes[2][2] = {
        {0, 4}, //blanco
        {7, 4}, // negro

    };



    // Constructor
    TableroAjedrez() {
        initializeBoard();
    }

    // auxiliar del constructor
    void initializeBoard() {
        char initial_board[8][8] = {
            {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'},//blancas
            {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
            {'.', '.', '.', '.', '.', '.', '.', '.'},
            {'.', '.', '.', '.', '.', '.', '.', '.'},
            {'.', '.', '.', '.', '.', '.', '.', '.'},
            {'.', '.', '.', '.', '.', '.', '.', '.'},
            {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
            {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'}//negras
        };
        //el tablero está al revés? si. Pero lo he hecho así por una razón muy específica:
        // las coordenadas del tablero coincidiran con las coordenadas de la matriz, lo que nos ahorra posibles quebraderos de cabeza.
        // ejemblo, la casilla d3 es la coordenada [3][4], lo que resulta logico y por ende facil de gestionar

        // c++ no me deja inicializar y luego asignar la matriz directamente asique lo haremos elemento a elemento
        for(int i = 0; i < 8; i++) {
            for(int j = 0; j < 8; j++) {
                tablero[i][j] = initial_board[i][j];
            }
        }
    }

    // metodo para mostrar el tablero. Haremos que muestre el trablero con las blancas abajo para no rayarnos.
    void display() {
        for (int i = 7; i > -1; i--) {
            for (int j = 0; j < 8; j++) {
                cout << tablero[i][j] << ' ';
            }
            cout << endl;
        }
    }


    bool isSuchRookMoveLegal( int x1, int y1, int x2, int y2){
        if (x1 == x2 || y1 == y2){
           if (x1 != x2){
                int menor;
                if (x1 > x2){
                    menor = x2;
                }
                else{
                    menor = x1;
                }
                for (int i = 1; i = abs(x1 - x2)-1; i++){
                    if (tablero[y1][menor + i] != '.'){
                        return false;
                    }
                }
            }
            else if(y1 != y2){
            int menor;
                if (y1 > y2){
                    menor = y2;
                }
                else{
                    menor = y1;
                }
                for (int i = 1; i = abs(y1 - y2)-1; i++){
                    if (tablero[menor + i][x1] != '.'){
                        return false;
                    }
                }
           }
        }
        else{
            return false;
        }

    }

    bool isSuchKingMoveLegal(int x1, int y1, int x2, int y2) {
        int aux = 1;
        char king = tablero[y1][x1];
        int deltaXp = x2 - x1;
        int deltaX = abs(x2 - x1);
        int deltaY = abs(y2 - y1);
        if (deltaXp<0){
            aux = -1;
        }
        // Una casilla, cualquier dirección
        if (deltaX <= 1 && deltaY <= 1) {
            // Comprobamos que no haya una pieza propia
            if (tablero[y2][x2] == '.') {
                return true;
            }
        }

        // Enroque
        if (deltaX == 2 && deltaY == 0) {
            if (king == 'K' && !whiteKingMoved) {
                if (x2 == 6 && !whiteRookMoved[1] && tablero[7][5] == '.' && tablero[7][6] == '.'&& isKingInCheck(true, x1, y1, 0, 0) && isKingInCheck(true, x1 + aux, y1, 0, 0)) {
                    // Enroque corto blancas
                    return true;
                } else if (x2 == 2 && !whiteRookMoved[0] && tablero[7][1] == '.' && tablero[7][2] == '.' && tablero[7][3] == '.'&&isKingInCheck(true, x1, y1, 0, 0) && isKingInCheck(true, x1 + aux, y1, 0, 0)) {
                    // Enroque largo blancas
                    return true;
                }
            } else if (king == 'k' && !blackKingMoved) {
                if (x2 == 6 && !blackRookMoved[1] && tablero[0][5] == '.' && tablero[0][6] == '.'&&isKingInCheck(false, x1, y1, 0, 0) && isKingInCheck(false, x1 + aux, y1, 0, 0)) {
                    // Enroque corto negras
                    return true;
                } else if (x2 == 2 && !blackRookMoved[0] && tablero[0][1] == '.' && tablero[0][2] == '.' && tablero[0][3] == '.'&& isKingInCheck(false, x1, y1, 0, 0) && isKingInCheck(false, x1 + aux, y1, 0, 0)) {
                    // Enroque largo negras
                    return true;
                }
            }
        }

        return false;
    }

    bool isSuchKnightMoveLegal(int x1, int y1, int x2, int y2){
        if((abs(x1 - x2) == 2 && abs(y1-y2)== 1) || (abs(x1 - x2) == 1 && abs(y1-y2)== 2)){
            return true;
        }
        else{
            return false;
        }
    }
    bool isSuchBishopMoveLegal(int x1, int y1, int x2, int y2){
        if (abs(x1-x2) == abs(y1-y2)){
            //comprobar que no se ha saltado ninguan pieza.
            if (x1 > x2){
               if(y1 > y2){
                    for(int i = 1; i = abs(x1-x2)-1; i++){
                        if (tablero[y2+i][x2 +i] != '.'){
                            return false;

                        }
                    }
               }
               else{
                    for(int i = 1; i = abs(x1-x2)-1; i++){
                        if (tablero[y1+i][x2 +i] != '.'){
                            return false;

                        }
                    }
               }
            }
            else{
                if(y1 > y2){
                for(int i = 1; i = abs(x1-x2)-1; i++){
                        if (tablero[y2+i][x1 +i] != '.'){
                            return false;

                        }
                    }
               }
               else{
                    for(int i = 1; i = abs(x1-x2)-1; i++){
                        if (tablero[y1+i][x1 +i] != '.'){
                            return false;

                        }
                    }
               }

            }
        }
        else{
            return false;
        }

    }
    bool isSuchQueenMoveLegal(int x1, int y1, int x2, int y2){
        if (abs(x1 - x2) == abs(y1-y2)){
            return isSuchBishopMoveLegal(x1, y1, x2, y2);
        }
        else {
            return isSuchRookMoveLegal(x1, y1, x2, y2);
        }
    }

    bool isSuchPawnMoveLegal(string move, int x1, int y1, int x2, int y2){
        if (white_turn){
            //blancas
            if (tablero[y2][x2] == '.'){
                //caso en que avanza
                if (y1 == 1){
                    if ((y1-y2 == 1 || y1 -y2 == 2)&& (x1 = x2)){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else if ((y1 == 4)&&(y2 == 5)&&(abs(x1-x2) == 1)&&(tablero[4][x2] == 'p')){
                        string ultimajugada = registroJugadas[numJugada];//por ajustar
                        int ultx2 = ultimajugada[2] - 97;
                        int ulty2 = static_cast<int>(ultimajugada[3]) - 49;
                        if ((ulty2 == 4) && (ultx2 == x2)){
                            return true;
                        }
                        else{
                            return false;
                        }

                }
                else if(y1 == 6 && y2 == 7){
                    char coronacion = move[4];
                    if(move.size() == 5 && (coronacion == 'N'|| coronacion == 'B'|| coronacion == 'Q'|| coronacion == 'R')){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else{
                   if ((y2-y1 == 1)&& (x1 = x2)){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
            }
            else{
                if((abs(x1-x2) == 1 ) && (y2-y1 == 1)){
                   return true;
                }
                else{
                    return false;
                }
            }
        }
        else{
            //negras
            if (tablero[y2][x2] == '.'){
                //caso en que avanza
                if (y1 == 6){
                    if ((y1-y2 == 1 || y1 -y2 == 2)&& (x1 = x2)){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else if ((y1 == 3)&&(y2 == 2)&&(abs(x1-x2) == 1)&&(tablero[3][x2] == 'P')){
                        string ultimajugada = registroJugadas[numJugada];//por ajustar
                        int ultx2 = ultimajugada[2] - 97;
                        int ulty2 = static_cast<int>(ultimajugada[3]) - 49;
                        if ((ulty2 == 3) && (ultx2 == x2)){
                            return true;
                        }
                        else{
                            return false;
                        }

                }
                else if(y1 == 1 && y2 == 0){
                    char coronacion = move[4];
                    if(move.size() == 5 && (coronacion == 'N'|| coronacion == 'B'|| coronacion == 'Q'|| coronacion == 'R')){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else{
                   if ((y1-y2 == 1)&& (x1 = x2)){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
            }
            else{
                if((abs(x1-x2) == 1 ) && (y1-y2 == 1)){
                   return true;
                }
                else{
                    return false;
                }
            }
        }

    }


    bool isSuchMovePseudoLegal(string move, int x1, int y1, int x2, int y2){
    if(x1 == y1 && x2 == y2){
        return false;
    }else{
        if ((x1 >= 0 && x1 <= 7) && (y1 >= 0 && y1 <= 7) && (x2 >= 0 && x2 <= 7) && (y2 >= 0 && y2 <= 7)){
            char pieza = tablero[y1][x1];
            if(pieza != '.'){
                if(white_turn){
                    int piez = static_cast<int>(pieza); //casting
                    if (piez < 96){
                        //vease tabla ascii, estamos comprobando que hay una letra mayuscula en la casilla de origen
                        char destino = tablero[y2][x2];
                        int dest = static_cast<int>(destino);
                        if(!(dest > 64) && (dest < 90)){
                            //comprobamos que no haya una letra mayuscula (pieza blanca) en la casilla de destino
                            switch(pieza){
                                case 'P':
                                    return isSuchPawnMoveLegal(move, x1, y1, x2, y2);
                                case 'B':
                                    return isSuchBishopMoveLegal(x1, y1, x2, y2);
                                case 'N':
                                    return isSuchKnightMoveLegal(x1, y1, x2, y2);
                                case 'R':
                                    return isSuchRookMoveLegal(x1, y1, x2, y2);
                                case 'Q':
                                    return isSuchQueenMoveLegal(x1, y1, x2, y2);
                                case 'K':
                                    return isSuchKingMoveLegal(x1, y1, x2, y2);
                                default:
                                    cout << "error detectando si " << move << " es legal" << endl;

                            }
                        }
                        else{
                            return false;
                        }
                    }
                    else{
                        return false;
                    }
                }
                else if(!white_turn){
                    // turno de las negras
                    int piez = static_cast<int>(pieza); //casting
                    if(piez > 96){

                        //vease tabla ascii, estamos comprobando que hay una letra ninuscula en la casilla de origen
                        char destino1 = tablero[y2][x2];
                        int dest1 = static_cast<int>(destino1);
                        if(!((dest1 > 96) && (dest1 < 123))){
                            //comprobamos que no haya una letra minuscula (pieza negra) del mismo color en dicha casilla
                            switch(pieza){
                                case 'p':
                                    return isSuchPawnMoveLegal(move, x1, y1, x2, y2);
                                case 'b':
                                    return isSuchBishopMoveLegal(x1, y1, x2, y2);
                                case 'n':
                                    return isSuchKnightMoveLegal(x1, y1, x2, y2);
                                case 'r':
                                    return isSuchRookMoveLegal(x1, y1, x2, y2);
                                case 'q':
                                    return isSuchQueenMoveLegal(x1, y1, x2, y2);
                                case 'k':
                                    return isSuchKingMoveLegal(x1, y1, x2, y2);
                                default:
                                    cout<< "error detectando si"<< move << "es legal"<<endl;
                            }

                        }
                        else{
                            return false;
                        }
                    }
                    else{
                        return false;
                    }

                }
            }
            else{
                return false;
            }
        }else{
            return false;
        }
        }
    }

    bool isKingInCheck(bool color, int xb, int yb, int xn, int yn){
        if (color){
            //caso1 le esta dando jaque un caballo
            //caso2 le esta dando jaque un alfil
            int xb1 = xb + 1;
            int yb1 = yb + 1;
            //caso jaque de peon
            if (tablero[yb1][xb1] == 'p'){
                return true;
            }
            //buscamos hacia arriba a la derecha
            while ((xb1 <= 7)&&(yb1 <= 7)&& ((tablero[yb1][xb1] == '.')|| (tablero[yb1][xb1] == 'b'|| tablero[yb1][xb1] == 'q'))) {
                if (tablero[yb1][xb1] == 'b'|| tablero[yb1][xb1] == 'q'){
                    return true;
                }
                xb1++;
                yb1++;
            }

            xb1 = xb - 1;
            yb1 = yb + 1;
            //caso jaque de peon
            if (tablero[yb1][xb1] == 'p'){
                return true;
            }
            //buscamos un alfil arriba a la izquierda
            while ((xb1 >= 0)&&(yb1 <= 7)&& ((tablero[yb1][xb1] == '.')|| (tablero[yb1][xb1] == 'b'|| tablero[yb1][xb1] == 'q'))) {
                if (tablero[yb1][xb1] == 'b'|| tablero[yb1][xb1] == 'q'){
                    return true;
                }
                xb1--;
                yb1++;
            }

            xb1 = xb + 1;
            yb1 = yb - 1;

            //buscamos un alfil abajo a la derecha
            while ((xb1 <= 7)&&(yb1 >= 0)&& ((tablero[yb1][xb1] == '.')|| (tablero[yb1][xb1] == 'b'|| tablero[yb1][xb1] == 'q'))) {
                if (tablero[yb1][xb1] == 'b'|| tablero[yb1][xb1] == 'q'){
                    return true;
                }
                xb1++;
                yb1--;
            }
            xb1 = xb - 1;
            yb1 = yb - 1;
            //buscamos un alfil abajo a la izqda

            while ((xb1 >= 0)&&(yb1 >= 0)&& ((tablero[yb1][xb1] == '.')|| (tablero[yb1][xb1] == 'b'|| tablero[yb1][xb1] == 'q'))) {
                if (tablero[yb1][xb1] == 'b'|| tablero[yb1][xb1] == 'q'){
                    return true;
                }
                xb1++;
                yb1--;
            }
            // caso del caballo
            // todas las posibles posiciones donde puede saltar:
            int posiblesSaltos[8][2] = {
            {xb + 2, yb + 1},
            {xb + 2, yb - 1},
            {xb - 2, yb + 1},
            {xb - 2, yb - 1},
            {xb + 1, yb + 2},
            {xb + 1, yb - 2},
            {xb - 1, yb + 2},
            {xb - 1, yb - 2}
            };
            for (int i = 0; i < 8; i++){
                if ((posiblesSaltos[i][0] >= 0 && posiblesSaltos[i][0] <= 7) &&(posiblesSaltos[i][1] >= 0 && posiblesSaltos[i][1] <= 7)){
                    if(tablero[posiblesSaltos[i][1]][posiblesSaltos[i][0]] == 'n'){
                        return true;
                    }
                }
            }
            //comprobar posible jaque de torre
            //comprobamos hacia arriba
            xb1 = xb;
            yb1 = yb + 1;
             while ((xb1 <= 7)&&(yb1 <= 7)&& (static_cast<int>(tablero[yb1][xb1]> 96))) {
                if (tablero[yb1][xb1] == 'r'|| tablero[yb1][xb1] == 'q'){
                    return true;
                }
                yb1++;
            }
            yb1 = yb;
            xb1 = xb - 1;


            //buscamos una torre a la izquierda
            while ((xb1 >= 0)&&(yb1 <= 7)&& ((tablero[yb1][xb1] == '.')|| (tablero[yb1][xb1] == 'r'|| tablero[yb1][xb1] == 'q'))) {
                if (tablero[yb1][xb1] == 'r'|| tablero[yb1][xb1] == 'q'){
                    return true;
                }
                xb1--;

            }

            xb1 = xb + 1;
            yb1 = yb;

            //buscamos una torre a la derecha
            while ((xb1 <= 7)&&(yb1 >= 0)&& ((tablero[yb1][xb1] == '.')|| (tablero[yb1][xb1] == 'r'|| tablero[yb1][xb1] == 'q'))) {
                if (tablero[yb1][xb1] == 'r'|| tablero[yb1][xb1] == 'q'){
                    return true;
                }
                xb1++;

            }
            xb1 = xb;
            yb1 = yb - 1;

            //buscamos una torre abajo

            while ((xb1 >= 0)&&(yb1 >= 0)&& ((tablero[yb1][xb1] == '.')|| (tablero[yb1][xb1] == 'r'|| tablero[yb1][xb1] == 'q'))) {
                if (tablero[yb1][xb1] == 'r'|| tablero[yb1][xb1] == 'q'){
                    return true;
                }

                yb1--;
            }
            //si todo este codigo se ejcuta es que no habra ninguna pieza que de jaque y entonces se devolvera false
            return false;

        }
        //caso negras
        else{
            //caso1 le esta dando jaque un caballo
            //caso2 le esta dando jaque un alfil
            int xn1 = xn + 1;
            int yn1 = yn + 1;
            //caso jaque de peon
            if (tablero[yn1][xn1] == 'P'){
                return true;
            }
            //buscamos hacia arriba a la derecha
            while ((xn1 <= 7)&&(yn1 <= 7)&& ((tablero[yn1][xn1] == '.')|| (tablero[yn1][xn1] == 'B'|| tablero[yn1][xn1] == 'Q'))) {
                if (tablero[yn1][xn1] == 'B'|| tablero[yn1][xn1] == 'Q'){
                    return true;
                }
                xn1++;
                yn1++;
            }

            xn1 = xn - 1;
            yn1 = yn + 1;
            //caso jaque de peon
            if (tablero[yn1][xn1] == 'P'){
                return true;
            }
            //buscamos un alfil arriba a la izquierda
            while ((xn1 >= 0)&&(yn1 <= 7)&& ((tablero[yn1][xn1] == '.')|| (tablero[yn1][xn1] == 'B'|| tablero[yn1][xn1] == 'Q'))) {
                if (tablero[yn1][xn1] == 'B'|| tablero[yn1][xn1] == 'Q'){
                    return true;
                }
                xn1--;
                yn1++;
            }

            xn1 = xn + 1;
            yn1 = yn - 1;

            //buscamos un alfil abajo a la derecha
            while ((xn1 <= 7)&&(yn1 >= 0)&& (((tablero[yn1][xn1] == '.')|| (tablero[yn1][xn1] == 'B'|| tablero[yn1][xn1] == 'Q')))) {
                if (tablero[yn1][xn1] == 'B'|| tablero[yn1][xn1] == 'Q'){
                    return true;
                }
                xn1++;
                yn1--;
            }
            xn1 = xn - 1;
            yn1 = yn - 1;
            //buscamos un alfil abajo a la izqda

            while ((xn1 >= 0)&&(yn1 >= 0)&& ((tablero[yn1][xn1] == '.')|| (tablero[yn1][xn1] == 'B'|| tablero[yn1][xn1] == 'Q'))) {
                if (tablero[yn1][xn1] == 'B'|| tablero[yn1][xn1] == 'Q'){
                    return true;
                }
                xn1++;
                yn1--;
            }
            // caso del caballo
            // todas las posibles posiciones donde puede saltar:
            int posiblesSaltos[8][2] = {
            {xn + 2, yn + 1},
            {xn + 2, yn - 1},
            {xn - 2, yn + 1},
            {xn - 2, yn - 1},
            {xn + 1, yn + 2},
            {xn + 1, yn - 2},
            {xn - 1, yn + 2},
            {xn - 1, yn - 2}
            };
            for (int i = 0; i < 8; i++){
                if ((posiblesSaltos[i][0] >= 0 && posiblesSaltos[i][0] <= 7) &&(posiblesSaltos[i][1] >= 0 && posiblesSaltos[i][1] <= 7)){
                    if(tablero[posiblesSaltos[i][1]][posiblesSaltos[i][0]] == 'N'){
                        return true;
                    }
                }
            }
            //comprobar posible jaque de torre
            //comprobamos hacia arriba
            xn1 = xn;
            yn1 = yn + 1;
             while ((xn1 <= 7)&&(yn1 <= 7)&& ((tablero[yn1][xn1] == '.')|| (tablero[yn1][xn1] == 'R'|| tablero[yn1][xn1] == 'Q'))) {
                if (tablero[yn1][xn1] == 'R'|| tablero[yn1][xn1] == 'Q'){
                    return true;
                }
                yn1++;
            }
            yn1 = yn;
            xn1 = xn - 1;


            //buscamos una torre a la izquierda
            while ((xn1 >= 0)&&(yn1 <= 7)&& ((tablero[yn1][xn1] == '.')|| (tablero[yn1][xn1] == 'R'|| tablero[yn1][xn1] == 'Q'))) {
                if (tablero[yn1][xn1] == 'R'|| tablero[yn1][xn1] == 'Q'){
                    return true;
                }
                xn1--;

            }

            xn1 = xn + 1;
            yn1 = yn;

            //buscamos una torre a la derecha
            while ((xn1 <= 7)&&(yn1 >= 0)&& ((tablero[yn1][xn1] == '.')|| (tablero[yn1][xn1] == 'R'|| tablero[yn1][xn1] == 'Q'))) {
                if (tablero[yn1][xn1] == 'R'|| tablero[yn1][xn1] == 'Q'){
                    return true;
                }
                xn1++;

            }
            xn1 = xn;
            yn1 = yn - 1;

            //buscamos una torre abajo

            while ((xn1 >= 0)&&(yn1 >= 0)&& ((tablero[yn1][xn1] == '.')|| (tablero[yn1][xn1] == 'R'|| tablero[yn1][xn1] == 'Q'))) {
                if (tablero[yn1][xn1] == 'R'|| tablero[yn1][xn1] == 'Q'){
                    return true;
                }

                yn1--;
            }
            //si todo este codigo se ejcuta es que no habra ninguna pieza que de jaque y entonces se devolvera false
            return false;

        }
    }
    string getLegalMoves(bool color){
        return "";
    }
    bool isCheckMate(bool color){
        return false;
    }
    bool isStalemate(bool color){
        return false;
    }
    string fromPositionToFEN(){
        string FEN = "";
        for (int i = 7; i > -1; i--){
            for(int j = 0; j< 8; j++){
                if(tablero[i][j] != '.'){
                    string p = "";          // Inicializamos un string vacío
                    p.push_back(tablero[i][j]); // Añadimos el carácter al string 'p'
                    FEN = FEN + p;
                }
                else{
                    int aux = 0;
                    while((tablero[i][j]== '.')&&(j <8)){
                        aux++;
                        j++;
                    }
                    string num = to_string(aux);
                    FEN = FEN + num;
                }
            }
            FEN = FEN + "/";
        }
        return FEN;
    }

    bool isGameOver(){
        bool gameOver = gameOverBy50MoveRule || gameOverByTFR || isStalemate(white_turn) || isCheckMate(white_turn);
        return gameOver;
    }

    void makeMove(string move, int x1, int y1, int x2, int y2) {
        char piece = tablero[y1][x1];
        char destino = tablero[y2][x2];
        //Si avanza un peon o hay una captura, reiniciamos la lista de TFR y el contador de la regla de las 50jugadas
        if(piece == 'P' || piece == 'p'|| destino != '.'){
            //reinicia el contador 50 jugadas @jose
            for(int i = 0; i< numJugadaTFR; i++){
                registroPosiciones[i][0] = "";
                registroPosiciones[i][1] = "";

            }
        }
        else{
            //avanza rl contador 50 jugadas
            //comprueba si ha llegado a 100
        }
        // Enroque
        if (piece == 'K' && abs(x2 - x1) == 2) {
            whiteKingMoved = true;
            if (x2 == 6) {
                // Enroque corto
                tablero[7][5] = 'R';
                tablero[7][7] = '.';
                tablero[y2][x2] = piece;
                tablero[y1][x1] = '.';
            } else if (x2 == 2) {
                // Enrpque largo
                tablero[7][3] = 'R';
                tablero[7][0] = '.';
                tablero[y2][x2] = piece;
                tablero[y1][x1] = '.';
            }
        } else if (piece == 'k' && abs(x2 - x1) == 2) {
            blackKingMoved = true;
            if (x2 == 6) {
                tablero[0][5] = 'r';
                tablero[0][7] = '.';
                tablero[y2][x2] = piece;
                tablero[y1][x1] = '.';
            } else if (x2 == 2) {
                tablero[0][3] = 'r';
                tablero[0][0] = '.';
                tablero[y2][x2] = piece;
                tablero[y1][x1] = '.';
            }
        }
        //coronacion blanca
        else if(piece == 'P' && x1 == 6 && x2 == 7){
            char coronacion = move[4]; // ultima letra de la jugada
            //@santi completa
        }
        // coronacion negra
        else if(piece == 'p' && x1 ==  && x2 == ){
            //@santi completa
        }

        else{
            tablero[y2][x2] = piece;
            tablero[y1][x1] = '.';
        }

        // Actualizr las flags(por ahora solo las del enroque)
        if (piece == 'K') {
            whiteKingMoved = true;
            cordReyes[0][0] = y2;
            cordReyes[0][1] = x2;

        }
        if (piece == 'k') {
            blackKingMoved = true;
            cordReyes[1][0] = y2;
            cordReyes[1][1] = x2;
        }
        if (piece == 'R' && x1 == 0) whiteRookMoved[0] = true;
        if (piece == 'R' && x1 == 7) whiteRookMoved[1] = true;
        if (piece == 'r' && x1 == 0) blackRookMoved[0] = true;
        if (piece == 'r' && x1 == 7) blackRookMoved[1] = true;

        // Registrar jugada
        registroJugadas[numJugada] = move;
        numJugada++;

        string FEN = fromPositionToFEN();
        bool posicionNoRepetida = true;
        // Registrar la posición
        for(int i = 0; i < numJugadaTFR; i++){
            if(registroPosiciones[i][0]== FEN){
                registroPosiciones[i][1] = registroPosiciones[i][1] + "I";
                string x = registroPosiciones[i][1];
                if (x.length() == 3){
                    gameOverByTFR = true;
                }
                posicionNoRepetida = false;
                break;
            }
        }
        if (posicionNoRepetida){
                registroPosiciones[numJugadaTFR][0] = FEN;
                registroPosiciones[numJugadaTFR][1] = "I";
                numJugadaTFR++;
        }
        white_turn = !white_turn;

    }

};


// Función para descomponer la jugada y retornar las coordenadas
    std::tuple<int, int, int, int> descomponerJugada(const std::string& move) {

    if (move.size() != 4) {
        throw std::invalid_argument("La jugada debe tener 4 caracteres.");
    }

    char xi = move[0]; // Letra inicial
    char yi = move[1]; // Número inicial
    char xf = move[2]; // Letra final
    char yf = move[3]; // Número final

    // Convertir los caracteres a números de coordenadas
    int nxi = xi - 97;
    int nyi = yi - 49;
    int nxf = xf - 97;
    int nyf = yf - 49;

    return std::make_tuple(nxi, nyi, nxf, nyf);
    }


void actualGame(TableroAjedrez tablero_principal){
    string moove;
    while (!tablero_principal.gameOver) {
        cout << "Enter your move: ";
        cin >> moove;
        if(moove.length() < 4){
            cout << "the move is too short"
            return actualGame();
        }
        int xi, yi, xf, yf;

        std::tie(xi, yi, xf, yf) = descomponerJugada(moove);

        if(tablero_principal.isSuchMovePseudoLegal(moove, xi, yi, xf, yf)){
            TableroAjedrez tablero_sec = tablero_principal;
            tablero_sec.makeMove(moove, xi, xf, yi, yf);
            int xk, yk;

            if (tablero_sec.white_turn){
                yk = tablero_sec.cordReyes[0][0];
                xk = tablero_sec.cordReyes[0][1];
                if(tablero_sec.isKingInCheck(tablero_sec.white_turn, xk, yk, xk, yk)){
                    tablero_principal.makeMove(moove, xi, yi, xf, yf);
                    return actualGame();
                }
                else{
                    cout << "illegal move youre walking into check";
                    return actualGame(tablero_principal);
                }
            }
            else{
                yk = tablero_sec.cordReyes[1][0];
                xk = tablero_sec.cordReyes[1][1];
                if(tablero_sec.isKingInCheck(tablero_sec.white_turn, xk, yk, xk, yk)){
                    tablero_principal.makeMove(moove, xi, yi, xf, yf);
                }
                else{
                    cout << "illegal move youre walking into check";
                    return actualGame(tablero_principal);
                }
            }

        }
        else{
            cout << "illegal move";
            return actualGame(tablero_principal);
        }

    }
}


int main() {
    TableroAjedrez tablero_principal;
    tablero_principal.display();
    actualGame(tablero_principal);
    return 0;
}
