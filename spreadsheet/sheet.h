#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet() {}

    void SetCell(Position pos, std::string text) override;

    CellInterface* GetCell(Position pos) override;
    const CellInterface* GetCell(Position pos) const override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    struct CellHasher {
        size_t operator()(const Position pos) const {
            return pos.row + 31 * pos.col;
        }
    };

    mutable std::unordered_map<Position, std::unique_ptr<Cell>, CellHasher> cells_;
};
