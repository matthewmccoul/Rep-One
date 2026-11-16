// Strict CSV reader (RFC 4180 style). Standard library only.
// Quoted fields may hold commas, doubled quotes and newlines. Ragged rows, an unterminated
// quote, stray quotes, empty/duplicate header names and an empty file are rejected.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace dd {
namespace csv {

struct Table {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;  // data rows, each header.size() fields
    std::vector<std::size_t> lines;              // 1-based line where each data row starts
    // Index of a header column, or -1.
    int column(const std::string& name) const;
};

struct Error {
    std::string code;  // CSV_EMPTY, CSV_RAGGED, CSV_OPEN_QUOTE, CSV_BAD_QUOTE, CSV_BAD_LINE_END, CSV_HEADER, CSV_MISSING_COLUMN
    std::string message;
    std::size_t line = 0;
    std::string describe() const;  // "line N: message"
};

// requiredColumns: header names that must be present (CSV_MISSING_COLUMN otherwise).
bool parse(const std::string& text, Table& out, Error& err,
           const std::vector<std::string>& requiredColumns = std::vector<std::string>());

}  // namespace csv
}  // namespace dd

#include "CoreMinimal.h"
