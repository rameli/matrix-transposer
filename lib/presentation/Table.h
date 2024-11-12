#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

class Table {
private:
    std::string title;
    std::string footer;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<size_t> columnWidths;
    size_t maxRows;

    void calculateColumnWidths() {
        // Determine the maximum width for each column
        columnWidths.resize(headers.size());
        for (size_t i = 0; i < headers.size(); ++i) {
            columnWidths[i] = headers[i].size();
            for (const auto& row : rows) {
                if (i < row.size()) {
                    columnWidths[i] = std::max(columnWidths[i], row[i].size());
                }
            }
        }
    }

    std::string createLine(char left, char fill, char middle, char right) const {
        std::ostringstream line;
        line << left;
        for (size_t i = 0; i < columnWidths.size(); ++i) {
            line << std::string(columnWidths[i] + 2, fill);
            if (i < columnWidths.size() - 1) line << middle;
        }
        line << right;
        return line.str();
    }

    std::string createRow(const std::vector<std::string>& row) const {
        std::ostringstream rowStream;
        rowStream << "|";
        for (size_t i = 0; i < columnWidths.size(); ++i) {
            rowStream << " " << std::setw(columnWidths[i]) << std::left 
                      << (i < row.size() ? row[i] : "") << " |";
        }
        return rowStream.str();
    }

public:
    Table(const std::string& title, const std::vector<std::string>& headers, size_t maxRows, const std::string& footer = "")
        : title(title), headers(headers), footer(footer), maxRows(maxRows)
        {
            rows.reserve(maxRows);
        }

    void addRow(const std::vector<std::string>& row) {
        if (rows.size() < maxRows) {
            rows.push_back(row);
        } else {
            std::cerr << "Error: Maximum row limit reached (" << maxRows << ").\n";
        }
    }

    void clear() {
        rows.clear();
    }

    std::string render() {
        calculateColumnWidths();

        std::ostringstream output;
        output << title << "\n";
        output << createLine('+', '-', '+', '+') << "\n";
        output << createRow(headers) << "\n";
        output << createLine('+', '-', '+', '+') << "\n";

        for (const auto& row : rows) {
            output << createRow(row) << "\n";
        }
        
        output << createLine('+', '-', '+', '+') << "\n";
        if (!footer.empty()) {
            output << footer << "\n";
        }
        return output.str();
    }
};