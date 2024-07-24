#include <iostream>
#include <string>

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
    bool isSuchRookMoveLegal(){
        return true;
    }
    bool isSuchKnightMoveLegal(){
        return true;
    }
    bool isSuchBishopMoveLegal(){
        return true;
    }
    bool isSuchQueenMoveLegal(){
        return (isSuchRookMoveLegal() ||  isSuchBishopMoveLegal());
    }
    bool isSuchKingMoveLegal(){
        return true;
    }
    bool isSuchPawnMoveLegal(){
        return true;
    }





    //falta terminar la funcion que determina si una jugada es legal
    bool isSuchMoveLegal(string move, int x1, int y1, int x2, int y2){
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
                                    return isSuchPawnMoveLegal();
                                case 'B':
                                    return isSuchBishopMoveLegal();
                                case 'N':
                                    return isSuchKnightMoveLegal();
                                case 'R':
                                    return isSuchRookMoveLegal();
                                case 'Q':
                                    return isSuchQueenMoveLegal();
                                case 'K':
                                    return isSuchKingMoveLegal();
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
                                    return isSuchPawnMoveLegal();
                                case 'b':
                                    return isSuchBishopMoveLegal();
                                case 'n':
                                    return isSuchKnightMoveLegal();
                                case 'r':
                                    return isSuchRookMoveLegal();
                                case 'q':
                                    return isSuchQueenMoveLegal();
                                case 'k':
                                    return isSuchKingMoveLegal();
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

