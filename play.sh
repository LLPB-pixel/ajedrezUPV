#!/bin/bash
# ──────────────────────────────────────────────────────
# play.sh — Script para jugar ajedrez contra el motor.
#
# Uso:
#   ./play.sh                 Jugar una partida normal
#   ./play.sh --fen "FEN"     Empezar desde una posición FEN
#   ./play.sh --depth 5       Usar profundidad 5 (default: 3)
#   ./play.sh --build         Compilar primero con make
#   ./play.sh --help          Mostrar ayuda
# ──────────────────────────────────────────────────────

set -euo pipefail
cd "$(dirname "$0")"

ENGINE="./bin/alphabeta_optimized"
FEN=""
DEPTH=""
BUILD=false

show_help() {
    cat <<EOF
♟  ajedrezUPV — Motor de ajedrez con IA

Uso:
  ./play.sh                 Jugar partida normal (profundidad 3)
  ./play.sh --fen "FEN"     Empezar desde posición FEN
  ./play.sh --depth N       Buscar a profundidad N (1-10)
  ./play.sh --build         Compilar antes de ejecutar
  ./play.sh --help          Mostrar esta ayuda

Comandos durante la partida:
  <movimiento>  Movimiento UCI (ej: e2e4, e7e8q, e8g8)
  d             Mostrar tablero
  fen           Mostrar FEN actual
  undo          Deshacer últimos 2 movimientos
  new           Nueva partida
  go depth N    Motor juega con profundidad N
  quit          Salir

Ejemplos:
  ./play.sh
  ./play.sh --depth 5
  ./play.sh --fen "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"
EOF
}

# Parse arguments.
while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h) show_help; exit 0 ;;
        --build)   BUILD=true; shift ;;
        --fen)     FEN="$2"; shift 2 ;;
        --depth)   DEPTH="$2"; shift 2 ;;
        *)         echo "Opción desconocida: $1"; show_help; exit 1 ;;
    esac
done

# Build if requested or if binary doesn't exist.
if $BUILD || [ ! -f "$ENGINE" ]; then
    echo "Compilando..."
    make engine -j$(nproc) 2>&1 | tail -3
    echo ""
fi

# Check binary exists.
if [ ! -f "$ENGINE" ]; then
    echo "Error: No se encontró el binario. Ejecuta: ./play.sh --build"
    exit 1
fi

# Build extra args.
ARGS=""
[ -n "$FEN" ] && ARGS="--fen \"$FEN\""
[ -n "$DEPTH" ] && ARGS="$ARGS --depth $DEPTH"

# Launch engine.
echo "♟  ajedrezUPV — Motor de ajedrez con IA"
echo "   Escribe 'help' para ver los comandos, 'quit' para salir."
echo ""
eval exec "$ENGINE" $ARGS
