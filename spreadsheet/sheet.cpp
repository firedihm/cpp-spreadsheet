#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    auto it = cells_.find(pos);
    if (it == cells_.end()) {
        it = cells_.emplace(pos, new Cell(*this)).first;
    }
    it->second->Set(text);
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    auto it = cells_.find(pos);
    if (it == cells_.end()) {
        return nullptr;
    }
    return it->second.get();
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    if (auto it = cells_.find(pos); it != cells_.end() && it->second != nullptr) {
        it->second->Clear();
        if (!it->second->IsReferenced()) {
            it->second.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size result;
    for (auto it = cells_.begin(); it != cells_.end(); ++it) {
        if (it->second != nullptr) {
            if (int row = it->first.row; result.rows <= row) {
                result.rows = row + 1;
            }
            if (int col = it->first.col; result.cols <= col) {
                result.cols = col + 1;
            }
        }
    }
    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col != 0) {
                output << '\t';
            }

            Position pos = { row, col };
            if (auto it = cells_.find(pos); it != cells_.end() && it->second != nullptr) {
                std::visit([&output](auto&& value) { output << value; }, it->second->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col != 0) {
                output << '\t';
            }

            Position pos = { row, col };
            if (auto it = cells_.find(pos); it != cells_.end() && it->second != nullptr) {
                output << it->second->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
