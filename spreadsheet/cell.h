#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <vector>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();
    void ClearCache();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    bool CuclicDependent(const Impl& impl)const;

    std::unique_ptr<Impl> impl_;

    // Добавьте поля и методы для связи с таблицей, проверки циклических 
    // зависимостей, графа зависимостей и т. д.
    Sheet& sheet_;
    std::unordered_set<Cell*> dependent_cells_;
    std::unordered_set<Cell*> referenced_cells_;
};