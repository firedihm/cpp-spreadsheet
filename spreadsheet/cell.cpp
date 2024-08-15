#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

namespace CellImpl {

struct Impl {
    virtual CellInterface::Value GetValue(SheetInterface&) const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
    virtual void ClearCache() = 0;
};

struct EmptyImpl : public Impl {
    CellInterface::Value GetValue(SheetInterface&) const override { return {}; }
    std::string GetText() const override { return {}; }
    std::vector<Position> GetReferencedCells() const override { return {}; }
    void ClearCache() override {}
};

struct TextImpl : public Impl {
    TextImpl(std::string_view text)
        : text_(text) {
    }

    CellInterface::Value GetValue(SheetInterface&) const override { return text_[0] == ESCAPE_SIGN ? text_.substr(1) : text_; }
    std::string GetText() const override { return text_; }
    std::vector<Position> GetReferencedCells() const override { return {}; }
    void ClearCache() override {}

private:
    std::string text_;
};

struct FormulaImpl : public Impl {
    FormulaImpl(std::string text)
        : formula_(ParseFormula(text)) {
    }

    CellInterface::Value GetValue(SheetInterface& sheet) const override {
        if (!cache_) {
            cache_ = formula_->Evaluate(sheet);
        }

        if (std::holds_alternative<double>(*cache_)) {
            return std::get<double>(*cache_);
        }
        return std::get<FormulaError>(*cache_);
    }
    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }
    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }
    void ClearCache() override {
        cache_.reset();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<FormulaInterface::Value> cache_;
};

} // namespace CellImpl

Cell::Cell(SheetInterface& sheet)
    : sheet_(sheet), impl_(std::make_unique<CellImpl::EmptyImpl>()) {
}

Cell::~Cell() {
}

void Cell::Set(std::string text) {
	if (text.empty()) {
        std::unique_ptr<CellImpl::EmptyImpl> new_impl(new CellImpl::EmptyImpl());

        ClearCache();
        RemoveReferences();

        impl_ = std::move(new_impl);
    } else if (text[0] == FORMULA_SIGN && text.size() > 1) {
        std::unique_ptr<CellImpl::FormulaImpl> new_impl(new CellImpl::FormulaImpl(text.substr(1)));

        std::vector<Position> new_referees(new_impl->GetReferencedCells());
		if (HasCircularDependency(new_referees)) {
			throw CircularDependencyException("Circular dependency detected");
		}

        ClearCache();
        RemoveReferences();
        AddReferences(new_referees);

        impl_ = std::move(new_impl);
    } else {
        std::unique_ptr<CellImpl::TextImpl> new_impl(new CellImpl::TextImpl(text));

        ClearCache();
        RemoveReferences();

        impl_ = std::move(new_impl);
    }
}

void Cell::Clear() {
	Set("");
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue(sheet_);
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !referrers_.empty();
}

void Cell::ClearCache() {
    impl_->ClearCache();

    for (Cell* referrer : referrers_) {
        referrer->ClearCache();
    }
}

void Cell::RemoveReferences() {
    for (Cell* referee : referees_) {
        referee->referrers_.erase(this);
    }
    referees_.clear();
}

void Cell::AddReferences(const std::vector<Position>& new_referees) {
    for (Position pos : new_referees) {
        Cell* cell = static_cast<Cell*>(sheet_.GetCell(pos));
        if (!cell) {
            sheet_.SetCell(pos, "");
        }

        cell->referrers_.insert(this);
        referees_.insert(cell);
    }
}

// вызывается из перегрузки ниже; рекурсивно проверяет иерархию ячеек на наличие циклической зависимости
bool Cell::HasCircularDependency(const std::unordered_set<const Cell*>& new_referees) const {
    if (new_referees.count(this)) { // contains() отсутствует в тренажёре?
        return true;
    }

    for (const Cell* referrer : referrers_) {
        if (referrer->HasCircularDependency(new_referees)) {
            return true;
        }
    }
    return false;
}

// здесь строим вспомогательное множество из указателей на ячейки
bool Cell::HasCircularDependency(const std::vector<Position>& new_referees_v) const {
    std::unordered_set<const Cell*> new_referees(new_referees_v.size());
    for (Position pos : new_referees_v) {
        new_referees.insert(static_cast<Cell*>(sheet_.GetCell(pos)));
    }

    return HasCircularDependency(new_referees);
}
