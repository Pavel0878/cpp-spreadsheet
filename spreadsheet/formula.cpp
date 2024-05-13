#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
   // return output << "#ARITHM!";
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression)
        : ast_(ParseFormulaAST(expression)){}

    Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(Position)> args = [&sheet](const Position pos) {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }
                if (sheet.GetCell(pos) == nullptr) { return 0.0; }
                const auto value = sheet.GetCell(pos)->GetValue();
                if (std::holds_alternative<double>(value)) {
                    return std::get<double>(value);
                }else 
                    if (std::holds_alternative<std::string>(value)) {
                        std::string tmp = std::get<std::string>(value);
                        if (tmp.empty()) { return 0.0; }
                        if (std::all_of(tmp.cbegin(), tmp.cend(), [](char c) {return (std::isdigit(c) || c == '.'); })) { return std::stod(tmp); }
                        else {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                    }
                    else {
                        throw FormulaError(FormulaError::Category::Arithmetic);
                    }
                return 0.0;
            };

        try {
            return ast_.Execute(args);

        }
        catch (const FormulaError& exc) {
            return exc;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> pos;
        for (const auto& p : ast_.GetCells()) {
            if (p.IsValid()) { pos.push_back(p); }
        }
        pos.resize(std::unique(pos.begin(), pos.end()) - pos.begin());
        return pos;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException("error");
    }
}