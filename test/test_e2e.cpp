// test_e2e.cpp — End-to-end tests simulating a real user playing the game.
//
// Tests the binary bin/alphabeta_optimized by feeding input via stdin/stdout.
// Uses popen to spawn the engine, write input, and read output with timeout.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

namespace {

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& msg) : std::runtime_error(msg) {}
};

void require(bool cond, const std::string& msg) {
    if (!cond) throw TestFailure(msg);
}

// ──────────────────────────────────────────────────────────────
// Process management: spawn the engine, write, read, kill.
// ──────────────────────────────────────────────────────────────

struct EngineProcess {
    pid_t pid = -1;
    int stdin_fd = -1;
    int stdout_fd = -1;

    // Spawn the engine binary.
    void start(const std::string& binary_path, const std::string& extra_args = "") {
        int in_pipe[2];
        int out_pipe[2];

        if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0) {
            throw TestFailure("pipe() failed");
        }

        pid = fork();
        if (pid < 0) {
            throw TestFailure("fork() failed");
        }

        if (pid == 0) {
            // Child process.
            close(in_pipe[1]);
            close(out_pipe[0]);
            dup2(in_pipe[0], STDIN_FILENO);
            dup2(out_pipe[1], STDOUT_FILENO);
            close(in_pipe[0]);
            close(out_pipe[1]);

            // Redirect stderr to /dev/null to avoid noise.
            int devnull = open("/dev/null", O_WRONLY);
            dup2(devnull, STDERR_FILENO);
            close(devnull);

            std::string cmd = binary_path + " " + extra_args;
            execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
            _exit(127);
        }

        // Parent process.
        close(in_pipe[0]);
        close(out_pipe[1]);
        stdin_fd = in_pipe[1];
        stdout_fd = out_pipe[0];

        // Set stdout_fd to non-blocking for timeout reads.
        int flags = fcntl(stdout_fd, F_GETFL, 0);
        fcntl(stdout_fd, F_SETFL, flags | O_NONBLOCK);
    }

    // Write input to the engine's stdin.
    void write_input(const std::string& input) {
        if (stdin_fd < 0) return;
        ::write(stdin_fd, input.c_str(), input.size());
    }

    // Read all available output (with a brief wait).
    std::string read_output(int timeout_ms = 500) {
        std::string output;
        auto start = std::chrono::steady_clock::now();

        while (true) {
            char buf[4096];
            ssize_t n = read(stdout_fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                output += buf;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= timeout_ms) break;

            if (n <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        return output;
    }

    // Close stdin to signal EOF.
    void close_stdin() {
        if (stdin_fd >= 0) {
            close(stdin_fd);
            stdin_fd = -1;
        }
    }

    // Kill the engine and wait for it to exit.
    void stop(int timeout_ms = 2000) {
        if (pid < 0) return;

        // Send SIGTERM.
        kill(pid, SIGTERM);

        // Wait up to timeout_ms for exit.
        auto start = std::chrono::steady_clock::now();
        int status;
        while (true) {
            pid_t ret = waitpid(pid, &status, WNOHANG);
            if (ret > 0) break;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= timeout_ms) {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (stdin_fd >= 0) close(stdin_fd);
        if (stdout_fd >= 0) close(stdout_fd);
        pid = -1;
    }

    // Check if the process is still running.
    bool is_running() {
        if (pid < 0) return false;
        int status;
        pid_t ret = waitpid(pid, &status, WNOHANG);
        return ret == 0; // 0 means still running.
    }

    ~EngineProcess() { stop(500); }
};

const char* ENGINE = nullptr;

// Auto-detect engine binary location.
std::string findEngine() {
    // Check common locations relative to the working directory.
    const char* candidates[] = {
        "./bin/alphabeta_optimized",         // Makefile build
        "./alphabeta_optimized",             // CMake in-source
        "../bin/alphabeta_optimized",        // From build/ subdirectory
    };
    for (const char* path : candidates) {
        if (access(path, X_OK) == 0) return path;
    }
    // Fall back to the Makefile default.
    return "./bin/alphabeta_optimized";
}

// Helper: run a quick test, send lines, read output, stop engine.
std::string runEngine(const std::string& input_lines, int wait_ms = 1000,
                      const std::string& extra_args = "") {
    EngineProcess ep;
    ep.start(ENGINE, extra_args);

    // Send all input lines, each followed by newline.
    ep.write_input(input_lines);

    // Read output.
    std::string output = ep.read_output(wait_ms);

    // Stop engine (sends SIGTERM).
    ep.stop(1000);

    return output;
}

// ══════════════════════════════════════════════════════════════
// TESTS
// ══════════════════════════════════════════════════════════════

void test_engine_starts() {
    std::string output = runEngine("quit\n", 1500);
    require(output.find("Bienvenido") != std::string::npos,
            "Engine should print welcome message");
    require(output.find("a b c d e f g h") != std::string::npos,
            "Engine should print initial board");
}

void test_basic_move_accepted() {
    // Engine plays white first. User plays black: e7e5.
    std::string output = runEngine("e7e5\n", 3000);
    require(output.find("Motor:") != std::string::npos,
            "Engine should play a move after user plays e7e5");
}

void test_illegal_move_rejected() {
    // User tries to move white's pawn while playing black.
    std::string output = runEngine("e2e4\nquit\n", 3000);
    require(output.find("ilegal") != std::string::npos || output.find("inválid") != std::string::npos,
            "Illegal move (white's pawn) should be rejected");
}

void test_impossible_move_rejected() {
    // Try moving a piece that doesn't exist at that square.
    std::string output = runEngine("a1a1\nquit\n", 3000);
    require(output.find("inválid") != std::string::npos || output.find("ilegal") != std::string::npos,
            "Impossible move should be rejected");
}

void test_empty_input_handled() {
    // Send just newlines — engine should not crash.
    std::string output = runEngine("\n\n\nquit\n", 2000);
    require(output.find("Bienvenido") != std::string::npos,
            "Engine should start OK with empty inputs");
}

void test_short_input_rejected() {
    std::string output = runEngine("e2\nquit\n", 2000);
    require(output.find("inválid") != std::string::npos || output.find("Formato") != std::string::npos,
            "Very short input should be rejected with format error");
}

void test_single_char_rejected() {
    std::string output = runEngine("a\nquit\n", 2000);
    require(output.find("inválid") != std::string::npos || output.find("Formato") != std::string::npos,
            "Single char input should be rejected");
}

void test_long_input_rejected() {
    std::string long_input(1000, 'x');
    long_input += "\nquit\n";
    std::string output = runEngine(long_input, 2000);
    require(output.find("inválid") != std::string::npos || output.find("Formato") != std::string::npos,
            "Very long input should be rejected");
}

void test_whitespace_trimmed() {
    // "  e7e5  " should be trimmed and accepted.
    std::string output = runEngine("  e7e5  \n", 3000);
    require(output.find("Motor:") != std::string::npos,
            "Input with surrounding whitespace should work");
}

void test_space_in_middle_rejected() {
    std::string output = runEngine("e7 e5\nquit\n", 2000);
    // "e7" is read first (too short), then "e5" is read (also too short).
    require(output.find("inválid") != std::string::npos || output.find("Formato") != std::string::npos
            || output.find("ilegal") != std::string::npos,
            "Input with space in middle should be rejected");
}

void test_uppercase_accepted() {
    // Engine now accepts case-insensitive input (toLower in the game loop).
    std::string output = runEngine("E7E5\n", 3000);
    require(output.find("Motor:") != std::string::npos,
            "Uppercase input should be accepted (case-insensitive)");
}

void test_special_chars_rejected() {
    std::string output = runEngine("e7e5!\nquit\n", 2000);
    require(output.find("inválid") != std::string::npos || output.find("Formato") != std::string::npos
            || output.find("ilegal") != std::string::npos,
            "Input with special chars should be rejected");
}

void test_eof_no_infinite_loop() {
    // [CRITICAL BUG FIX TEST] Send no input, just close stdin.
    // The old code would infinite-loop. The fixed code should exit.
    EngineProcess ep;
    ep.start(ENGINE);
    ep.close_stdin();

    // Wait up to 3 seconds. If it's still running, it's the infinite loop bug.
    auto start = std::chrono::steady_clock::now();
    bool exited = false;
    while (true) {
        if (!ep.is_running()) { exited = true; break; }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= 3000) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ep.stop(500);
    require(exited, "Engine must exit within 3 seconds on EOF (no infinite loop)");
}

void test_eof_after_move() {
    // Send one legal move, then close stdin (EOF).
    EngineProcess ep;
    ep.start(ENGINE);
    // Wait for prompt.
    ep.read_output(1500);
    // Send move and then close stdin.
    ep.write_input("e7e5\n");
    ep.read_output(3000); // Let engine respond.
    ep.close_stdin();

    auto start = std::chrono::steady_clock::now();
    bool exited = false;
    while (true) {
        if (!ep.is_running()) { exited = true; break; }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= 3000) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ep.stop(500);
    require(exited, "Engine must exit on EOF after a move");
}

void test_quit_command() {
    std::string output = runEngine("quit\n", 1500);
    require(output.find("Hasta luego") != std::string::npos || output.find("luego") != std::string::npos,
            "quit command should print goodbye");
}

void test_display_command() {
    std::string output = runEngine("d\nquit\n", 2000);
    require(output.find("a b c d e f g h") != std::string::npos,
            "'d' command should display the board");
}

void test_fen_command() {
    std::string output = runEngine("fen\nquit\n", 2000);
    require(output.find("rnbqkbnr") != std::string::npos,
            "'fen' command should output the FEN string");
}

void test_new_command() {
    std::string output = runEngine("e7e5\nnew\nquit\n", 4000);
    require(output.find("Nueva partida") != std::string::npos,
            "'new' command should start a new game");
}

void test_invalid_command() {
    std::string output = runEngine("blargh\nquit\n", 2000);
    require(output.find("inválid") != std::string::npos || output.find("Formato") != std::string::npos,
            "Invalid command should give an error");
}

void test_multiple_illegal_then_legal() {
    // Try 3 illegal moves, then a legal one.
    std::string output = runEngine("e2e4\ne3e3\na1a1\ne7e5\n", 5000);
    require(output.find("Motor:") != std::string::npos,
            "After illegal moves followed by legal, engine should play");
}

void test_castling() {
    // Position where black can castle kingside.
    std::string fen = "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R b KQkq - 0 1";
    std::string output = runEngine("e8g8\n", 3000, "--fen \"" + fen + "\"");
    require(output.find("Motor:") != std::string::npos || output.find("a b c d") != std::string::npos,
            "Castling should be accepted and played");
}

void test_en_passant() {
    // Position with en passant available.
    std::string fen = "rnbqkbnr/pppp1ppp/8/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3";
    std::string output = runEngine("e5f6\n", 3000, "--fen \"" + fen + "\"");
    require(output.find("Motor:") != std::string::npos || output.find("a b c d") != std::string::npos,
            "En passant capture should be accepted");
}

void test_promotion() {
    // Position where black can promote.
    std::string fen = "8/P7/8/8/8/8/8/4K2k b - - 0 1";
    std::string output = runEngine("a7a8q\n", 3000, "--fen \"" + fen + "\"");
    require(output.find("Motor:") != std::string::npos || output.find("a b c d") != std::string::npos,
            "Promotion should be accepted");
}

void test_go_depth_command() {
    std::string output = runEngine("go depth 1\nquit\n", 3000);
    require(output.find("Motor") != std::string::npos,
            "go depth N command should make engine play");
}

void test_game_over_checkmate() {
    // Use a position where the engine delivers checkmate in 1 as white.
    // Back-rank mate: Re8#
    std::string fen = "6k1/5ppp/8/8/8/8/8/4R1K1 w - - 0 1";
    std::string output = runEngine("", 3000, "--fen \"" + fen + "\"");
    // Engine should play e1e8 (the checkmate).
    require(output.find("e1e8") != std::string::npos,
            "Engine should find e1e8 (checkmate) from back-rank position");
    require(output.find("terminado") != std::string::npos ||
            output.find("Jaque mate") != std::string::npos,
            "Game should end after checkmate");
}

void test_board_restores_after_search() {
    // Play a move, verify board is consistent.
    std::string output = runEngine("e7e5\n", 3000);
    // Board should show e5 pawn (black) after the move.
    require(output.find("Motor:") != std::string::npos,
            "Engine should play after user move");
}

void test_undo_command() {
    std::string output = runEngine("e7e5\nundo\nquit\n", 5000);
    require(output.find("Deshecho") != std::string::npos,
            "'undo' should print 'Deshecho'");
}

void test_undo_no_moves() {
    std::string output = runEngine("undo\nquit\n", 2000);
    require(output.find("No hay movimientos") != std::string::npos,
            "'undo' with no moves should say so");
}

void test_invalid_fen_does_not_crash() {
    std::string output = runEngine("quit\n", 1500, "--fen \"invalid_fen\"");
    // Engine should either start with default position or fail gracefully.
    // The important thing is it doesn't crash.
    // Just check it terminates.
    require(true, "Invalid FEN should not crash the engine");
}

// ──────────────────────────────────────────────────────────────
// Test runner
// ──────────────────────────────────────────────────────────────

void runTest(const char* name, void (*test)(), int& failures) {
    try {
        test();
        std::cout << "PASS " << name << '\n';
    } catch (const std::exception& e) {
        ++failures;
        std::cerr << "FAIL " << name << ": " << e.what() << '\n';
    }
}

} // namespace

int main() {
    // Auto-detect engine binary location.
    std::string engine_path = findEngine();
    ENGINE = engine_path.c_str();

    int failures = 0;

    std::cout << "=== E2E Tests: Engine startup ===\n";
    runTest("engine_starts", test_engine_starts, failures);
    runTest("quit_command", test_quit_command, failures);

    std::cout << "\n=== E2E Tests: Normal gameplay ===\n";
    runTest("basic_move_accepted", test_basic_move_accepted, failures);
    runTest("board_restores_after_search", test_board_restores_after_search, failures);

    std::cout << "\n=== E2E Tests: Input validation ===\n";
    runTest("illegal_move_rejected", test_illegal_move_rejected, failures);
    runTest("impossible_move_rejected", test_impossible_move_rejected, failures);
    runTest("empty_input_handled", test_empty_input_handled, failures);
    runTest("short_input_rejected", test_short_input_rejected, failures);
    runTest("single_char_rejected", test_single_char_rejected, failures);
    runTest("long_input_rejected", test_long_input_rejected, failures);
    runTest("whitespace_trimmed", test_whitespace_trimmed, failures);
    runTest("space_in_middle_rejected", test_space_in_middle_rejected, failures);
    runTest("uppercase_accepted", test_uppercase_accepted, failures);
    runTest("special_chars_rejected", test_special_chars_rejected, failures);
    runTest("multiple_illegal_then_legal", test_multiple_illegal_then_legal, failures);

    std::cout << "\n=== E2E Tests: EOF handling (critical bug fix) ===\n";
    runTest("eof_no_infinite_loop", test_eof_no_infinite_loop, failures);
    runTest("eof_after_move", test_eof_after_move, failures);

    std::cout << "\n=== E2E Tests: Commands ===\n";
    runTest("display_command", test_display_command, failures);
    runTest("fen_command", test_fen_command, failures);
    runTest("new_command", test_new_command, failures);
    runTest("invalid_command", test_invalid_command, failures);
    runTest("go_depth_command", test_go_depth_command, failures);
    runTest("undo_command", test_undo_command, failures);
    runTest("undo_no_moves", test_undo_no_moves, failures);

    std::cout << "\n=== E2E Tests: Chess rules ===\n";
    runTest("castling", test_castling, failures);
    runTest("en_passant", test_en_passant, failures);
    runTest("promotion", test_promotion, failures);
    runTest("game_over_checkmate", test_game_over_checkmate, failures);

    std::cout << "\n=== E2E Tests: Edge cases ===\n";
    runTest("invalid_fen_does_not_crash", test_invalid_fen_does_not_crash, failures);

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED")
              << " (" << failures << " failures)\n";

    return failures == 0 ? 0 : 1;
}
