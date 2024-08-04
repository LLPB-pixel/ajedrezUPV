#include <iostream>
#include <string>
#include <cmath>
using namespace std;

class TableroAjedrez {

public:
    // Attribute
    char tablero[8][8];
    string registroJugadas[400]; //estos dos atributos estan por si acos queremos generar una funcion que deshaga jugadas
    bool white_turn = true;
    int numJugada = 0;

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



    // Se que este metodo es un poco raro, pero actua como diccionario
    // Si pones, devuelve
    // a    ----> 0
    // b    ----> 1
    // c    ----> 2
    // Y asi sucesivamente. Sera muy util para relacionar jugadas con casillas del tablero virtual que hemos creado.

    int dictionary(char letra){
        int ascii_code;
        ascii_code = static_cast<int>(letra);
        return ascii_code - 97;
    }
    int descomponerJugada(string move){
        char letrasJugada[4];
        for (int i = 0; i < 4; ++i) {
            letrasJugada[i] = move[i];
        }
        char xi = letrasJugada[0]; //letra inicial
        char yi = letrasJugada[1]; //numero inicial
        char xf = letrasJugada[2]; //letra final
        char yf = letrasJugada[3]; //numero final

        // Convierto los chars a int y les resto 1 pq en programcion elprimer numero es el 0.
        int nxi = dictionary(xi); //aqui no hace falta porque en la funcion dictionary ya se hace
        int nyi = yi - '0' - 1; // estoy convirtiendo el caracter alfanumerico en un numero explotando una caracteristica de la tabla ascii
        int nxf = dictionary(xf);//aqui no hace falta porque en la funcion dictionary ya se hace
        int nyf = yf - '0' - 1; // estoy convirtiendo el caracter alfanumerico en un numero explotando una caracteristica de la tabla ascii

        return nxi, nyi, nxf, nyf;

    }
    //he puesto return true para que esta parte compile y poder depurar el código
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
                for (int i = 1; i = abs(x1 - x2); i++){
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
                for (int i = 1; i = abs(y1 - y2); i++){
                    if (tablero[menor + i][x1] != '.'){
                        return false;
                    }
                }
           }
        }
        else{
            return false;
        }
        return true;
    }
    bool isSuchKnightMoveLegal(int x1, int y1, int x2, int y2){
        if((abs(x1 - x2) == 2 && abs(y1-y2)== 1) || (abs(x1 - x2) == 1 && abs(y1-y2)== 2))
            return true;
        else{
            return false;
        }
    }
    bool isSuchBishopMoveLegal(int x1, int y1, int x2, int y2){
        if (abs(x1-x2) == abs(y1-y2)){
            //comprobar que no se ha saltado ninguan pieza.
            if (x1 > x2){
               if(y1 > y2){
                    for(int i = 1; i = abs(x1-x2); i++){
                        if (tablero[y2+i][x2 +i] != '.'){
                            return false;

                        }
                    }
               }
               else{
                    for(int i = 1; i = abs(x1-x2); i++){
                        if (tablero[y1+i][x2 +i] != '.'){
                            return false;

                        }
                    }
               }
            }
            else{
                if(y1 > y2){
                for(int i = 1; i = abs(x1-x2); i++){
                        if (tablero[y2+i][x1 +i] != '.'){
                            return false;

                        }
                    }
               }
               else{
                    for(int i = 1; i = abs(x1-x2); i++){
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
    bool isSuchQueenMoveLegal( int x1, int y1, int x2, int y2){
        if (abs(x1 - x2) == abs(y1-y2)){
            return isSuchBishopMoveLegal(x1, y1, x2, y2);
        }
        else {
            return isSuchRookMoveLegal(x1, y1, x2, y2);
        }
    }

    bool isSuchKingMoveLegal(bool color, int x1, int y1, int x2, int y2){
        return true;
    }
    bool isSuchPawnMoveLegal(bool color, int x1, int y1, int x2, int y2){
        return true;
    }





    //falta terminar la funcion que determina si una jugada es legal
    bool isSuchMoveLegal(string move, int x1, int y1, int x2, int y2){
    if(x1 == y1 && x2 == y2){
        return false;
    }else{
        if ((x1 >= 0 && x1 <= 7) && (y1 >= 0 && y1 <= 7) && (x2 >= 0 && x2 <= 7) && (y2 >= 0 && y2 <= 7)){
            char pieza = tablero[y1][x1];
            if(pieza != '.'){
                if(white_turn){
                    bool color = true;
                    int piez = static_cast<int>(pieza); //casting
                    if (piez < 96){
                        //vease tabla ascii, estamos comprobando que hay una letra mayuscula en la casilla de origen
                        char destino = tablero[y2][x2];
                        int dest = static_cast<int>(destino);
                        if(!(dest > 64) && (dest < 90)){
                            //comprobamos que no haya una letra mayuscula (pieza blanca) en la casilla de destino
                            switch(pieza){
                                case 'P':
                                    return isSuchPawnMoveLegal(color, x1, y1, x2, y2);
                                case 'B':
                                    return isSuchBishopMoveLegal(x1, y1, x2, y2);
                                case 'N':
                                    return isSuchKnightMoveLegal(x1, y1, x2, y2);
                                case 'R':
                                    return isSuchRookMoveLegal(x1, y1, x2, y2);
                                case 'Q':
                                    return isSuchQueenMoveLegal( x1, y1, x2, y2);
                                case 'K':
                                    return isSuchKingMoveLegal(color, x1, y1, x2, y2);
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
                else if(!white_turn){
                    bool color = false;
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
                                    return isSuchPawnMoveLegal(color, x1, y1, x2, y2);
                                case 'b':
                                    return isSuchBishopMoveLegal(x1, y1, x2, y2);
                                case 'n':
                                    return isSuchKnightMoveLegal(x1, y1, x2, y2);
                                case 'r':
                                    return isSuchRookMoveLegal(x1, y1, x2, y2);
                                case 'q':
                                    return isSuchQueenMoveLegal( x1, y1, x2, y2);
                                case 'k':
                                    return isSuchKingMoveLegal(color, x1, y1, x2, y2);
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

    void makeMove(string move, int x1, int y1, int x2, int y2){
        //Precondicion: la jugada tiene que ser legal
        registroJugadas[numJugada] = move;
        numJugada++;

        tablero[y2][x2]= tablero[y1][x1];
        tablero[y1][x1] = '.';
    }



};

int main() {
    TableroAjedrez tablero;
    tablero.display();
    return 0;
}
