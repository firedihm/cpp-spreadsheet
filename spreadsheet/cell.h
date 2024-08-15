#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>

namespace CellImpl {
struct Impl;
}

class Cell : public CellInterface {
public:
    Cell(SheetInterface&);
    ~Cell();

    void Set(std::string);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    void ClearCache(); // очищает кэш ячеек, ссылающихся на эту
    void RemoveReferences(); // удаляет ссылки на эту ячейку у всех ссылаемых ячеек
    void AddReferences(const std::vector<Position>&);

    bool HasCircularDependency(const std::unordered_set<const Cell*>&) const;
    bool HasCircularDependency(const std::vector<Position>&) const;

    SheetInterface& sheet_;
    std::unique_ptr<CellImpl::Impl> impl_;

    std::unordered_set<Cell*> referrers_; // ячейки, ссылающиеся на эту
    std::unordered_set<Cell*> referees_; // ячейки, на которые эта ссылается
};
