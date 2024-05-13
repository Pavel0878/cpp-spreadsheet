#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (pos.IsValid()) {
        if (cells_.end() == cells_.find(pos)) { cells_.emplace(pos, std::make_unique<Cell>(*this)); }
        cells_.at(pos)->Set(text);
        size_ = GetPrintableSize();
    }
    else {
        throw InvalidPositionException("Invalid position, SetCell");
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (pos.IsValid()) {
        if (cells_.count(pos)) { return cells_.at(pos).get(); }
        return nullptr;
    }
    else {
        throw InvalidPositionException("Invalid position, GetCell");
    }
}
CellInterface* Sheet::GetCell(Position pos) {
    if (pos.IsValid()) {
        if (cells_.count(pos)) { return cells_.at(pos).get(); }
        return nullptr;
    }
    else {
        throw InvalidPositionException("Invalid position, GetCell");
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) { throw InvalidPositionException("Invalid position, ClearCell"); }
    const auto& cell = cells_.find(pos);
    if (cell != cells_.end() && cell->second != nullptr) {
        cell->second->Clear();
        if (!cell->second->IsReferenced()) {
            cell->second.reset();
        }
    }
    size_ = GetPrintableSize();
}

Size Sheet::GetPrintableSize() const {
    Size result;

    for (auto it = cells_.begin(); it != cells_.end(); ++it) {
        if (it->second != nullptr && !it->second->GetText().empty()) {
            const int col = it->first.col;
            const int row = it->first.row;
            result.rows = std::max(result.rows, row + 1);
            result.cols = std::max(result.cols, col + 1);
        }
    }
    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int row = 0; row < size_.rows; ++row) {
        for (int col = 0; col < size_.cols; ++col) {
            if (col) { output << '\t'; }
            const auto cell = cells_.find({ row, col });
            if (cell != cells_.end() && cell->second != nullptr) {
                std::visit([&output](const auto& arg) {output << arg; }, cell->second->GetValue());
            }
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    for (int row = 0; row < size_.rows; ++row) {
        for (int col = 0; col < size_.cols; ++col) {
            if (col) { output << '\t'; }
            const auto cell = cells_.find({ row, col });
            if (cell != cells_.end() && cell->second != nullptr) {
                output << cell->second->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}