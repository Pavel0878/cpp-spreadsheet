#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <cmath>

class Cell::Impl {
public:
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const { return {}; }
    virtual void ResetCache(){}
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override { return 0.0; };
    std::string GetText() const override { return ""; };
};

class Cell::TextImpl : public Impl {
public:
    explicit TextImpl(std::string text)
        : text_(text) {}

    Value GetValue() const override {
        if (text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }
    std::string GetText() const override {
        return text_;
    }
private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string text, const SheetInterface& sheet)
        : formula_(ParseFormula(text))
        , sheet_(sheet){}

    Value GetValue() const override {
        
        if (!cache_) {
            FormulaInterface::Value result = formula_->Evaluate(sheet_);
            if (std::holds_alternative<double>(result)){
                if (std::isfinite(std::get<double>(result))) {
                    return std::get<double>(result);
                }
                else{
                    return FormulaError(FormulaError::Category::Arithmetic);
                }
            }
            return std::get<FormulaError>(result);
        }
        return *cache_;
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

    void ResetCache() override {
        if (cache_.has_value()) {
            cache_.reset();
        }
    }

private:
    std::optional<CellInterface::Value> cache_;
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface& sheet_;};

bool Cell::CuclicDependent(const Impl& impl) const {
    std::vector<Position> vector_pos = impl.GetReferencedCells();
    if (!vector_pos.empty()) {
        std::unordered_set<const Cell*> ref;
        for (Position& pos : vector_pos) {
            ref.insert(const_cast<Cell*>(dynamic_cast<const Cell*>(sheet_.GetCell(pos))));
        }
        std::unordered_set<const Cell*> verified;
        std::vector<const Cell*> visited;
        visited.push_back(this);
        while (!visited.empty()) {
            const Cell* cell = visited.back();
            visited.pop_back();
            verified.insert(cell);
            if (ref.find(cell) != ref.end()) { return true; }
            for (const Cell* dep : cell->dependent_cells_) {
                if (verified.find(dep) == verified.end()) { visited.push_back(dep); }
            }
        }
    }
    return false;
}

// Реализуйте следующие методы
Cell::Cell(Sheet& sheet)
	: impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet){}

Cell::~Cell(){}

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> impl;
	if (text.empty()) {
		impl = std::make_unique<EmptyImpl>();
	}
	else if (text[0] == FORMULA_SIGN /* && text.size() > 1*/) {
        try {
            impl = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
        }
        catch (...) { throw FormulaException("error"); }
	}
	else {
		impl = std::make_unique<TextImpl>(text);
	}

    if (CuclicDependent(*impl)) { throw CircularDependencyException("Cuclic"); }
    impl_ = std::move(impl);

    for (Cell* ref : referenced_cells_) {
        ref->dependent_cells_.erase(this);
    }
    referenced_cells_.clear();
    for (const auto& pos : impl_->GetReferencedCells()) {
        CellInterface* cell_tmp = sheet_.GetCell(pos);
        if (!cell_tmp) {
            sheet_.SetCell(pos, "");
            cell_tmp = sheet_.GetCell(pos);
        }
        Cell* cell = const_cast<Cell*>(dynamic_cast<const Cell*>(cell_tmp));
        referenced_cells_.insert(cell);
        cell->dependent_cells_.insert(this);
    }
    ClearCache();
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}

void Cell::ClearCache() {
    impl_->ResetCache();
    for (auto& cell : dependent_cells_) {
        cell->ClearCache();
    }
}

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }
std::string Cell::GetText() const { return impl_->GetText(); }
std::vector<Position> Cell::GetReferencedCells() const { return impl_->GetReferencedCells(); }
bool Cell::IsReferenced() const { return !dependent_cells_.empty(); }