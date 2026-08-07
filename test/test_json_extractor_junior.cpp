#define main json_extractor_junior_program_main
#include "../src/tools/json_extractor_junior.cpp"
#undef main

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void validLineProducesFormattedRecords() {
    const std::string line = R"({
        "fen": "8/8/8/8/8/8/8/K6k w - - 0 1",
        "evals": [{
            "pvs": [
                {"cp": 42},
                {"cp": -13},
                {"line": "a1b1"}
            ]
        }]
    })";

    std::vector<Registro> records;
    procesarLinea(line, records);

    require(records.size() == 2, "valid JSON line did not produce two records");
    require(records[0].fen == "8/8/8/8/8/8/8/K6k w - - 0 1" &&
                records[1].fen == records[0].fen,
            "record FEN has the wrong format");
    require(records[0].cp == 42 && records[1].cp == -13,
            "centipawn values were not preserved");
}

void incompleteEntriesAreIgnored() {
    std::vector<Registro> records;
    procesarLinea(R"({
        "fen": "8/8/8/8/8/8/8/K6k w - - 0 1",
        "evals": [{"pvs": [{"line": "a1b1"}]}]
    })",
                  records);
    require(records.empty(),
            "entries without a centipawn value were not ignored");

    procesarLinea(R"({"fen": "8/8/8/8/8/8/8/K6k w - - 0 1", "evals": [null]})",
                  records);
    require(records.empty(), "invalid evaluation object was not ignored");
}

void fenConversionHasTheExpectedShape() {
    const std::string whiteFen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    const std::string blackFen = "8/8/8/8/8/8/8/K6k b - - 0 1";
    float whiteInput[773];
    float blackInput[773];

    convertirFENaInput(whiteFen, whiteInput);
    convertirFENaInput(blackFen, blackInput);

    int whitePieces = 0;
    int blackPieces = 0;
    for (int index = 0; index < 768; ++index) {
        require(whiteInput[index] == 0.0f || whiteInput[index] == 1.0f,
                "piece input is not one-hot encoded");
        require(blackInput[index] == 0.0f || blackInput[index] == 1.0f,
                "piece input is not one-hot encoded");
        whitePieces += whiteInput[index] == 1.0f;
        blackPieces += blackInput[index] == 1.0f;
    }

    require(whitePieces == 6 && blackPieces == 2,
            "piece channels do not contain the expected pieces");
    for (int index = 768; index < 772; ++index) {
        require(whiteInput[index] == 1.0f,
                "white castling rights are not encoded as one-hot values");
        require(blackInput[index] == 0.0f,
                "absent castling rights are not encoded as zero");
    }
    require(whiteInput[772] == 1.0f && blackInput[772] == 0.0f,
            "side to move is not encoded in the final input slot");

    for (float value : whiteInput) {
        require(std::isfinite(value), "white input contains a non-finite value");
    }
    for (float value : blackInput) {
        require(std::isfinite(value), "black input contains a non-finite value");
    }
}

void runTest(const char* name, void (*test)(), int& failures) {
    try {
        test();
        std::cout << "PASS " << name << '\n';
    } catch (const std::exception& error) {
        ++failures;
        std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    }
}

} // namespace

int main() {
    int failures = 0;
    runTest("valid_line_produces_formatted_records",
            validLineProducesFormattedRecords, failures);
    runTest("incomplete_entries_are_ignored", incompleteEntriesAreIgnored,
            failures);
    runTest("fen_conversion_has_the_expected_shape", fenConversionHasTheExpectedShape,
            failures);
    return failures == 0 ? 0 : 1;
}
