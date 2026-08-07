#define main json_extractor_program_main
#include "../src/tools/json_extractor.cpp"
#undef main

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
                {"line": "a1b1", "cp": 12},
                {"line": "a1a2", "cp": -7}
            ]
        }]
    })";

    std::vector<Registro> records;
    procesarLinea(line, records);

    require(records.size() == 2, "valid JSON line did not produce two records");
    for (const Registro& record : records) {
        require(record.fen == "8/8/8/8/8/8/8/K6k w - - 0 1",
                "record FEN has the wrong format");
        require(!record.line.empty(), "record line is empty");
    }
    require(records[0].cp == 12 && records[1].cp == -7,
            "centipawn values were not preserved");
}

void incompleteEntriesAreIgnored() {
    const std::string line = R"({
        "fen": "8/8/8/8/8/8/8/K6k w - - 0 1",
        "evals": [{
            "pvs": [
                {"line": "a1b1"},
                {"cp": 10},
                {"line": "a1a2", "cp": 20}
            ]
        }]
    })";

    std::vector<Registro> records;
    procesarLinea(line, records);

    require(records.size() == 1,
            "entries without the required output fields were not ignored");
    require(records[0].line == "a1a2" && records[0].cp == 20,
            "the complete record has the wrong format");
}

void missingTopLevelFieldsAreIgnored() {
    std::vector<Registro> records;
    procesarLinea(R"({"evals": []})", records);
    procesarLinea(R"({"fen": "8/8/8/8/8/8/8/K6k w - - 0 1"})", records);
    procesarLinea(R"({"fen": "8/8/8/8/8/8/8/K6k w - - 0 1", "evals": [null]})",
                  records);
    require(records.empty(), "invalid top-level JSON was converted to a record");
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
    runTest("missing_top_level_fields_are_ignored", missingTopLevelFieldsAreIgnored,
            failures);
    return failures == 0 ? 0 : 1;
}
