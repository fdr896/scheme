#pragma once

#include <algorithm>
#include <string>
#include <map>
#include <memory>
#include "error.h"
#include <functional>
#include <vector>

class Scope;

class Object : public std::enable_shared_from_this<Object> {
public:
    virtual ~Object() = default;

    inline virtual std::shared_ptr<Object> Eval(Scope&) {
        throw RuntimeError{"not evaluative object"};
    }

    inline virtual std::string Stringify() {
        throw RuntimeError{"object can not be stringified"};
    }

    inline virtual std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) {
        throw RuntimeError{"not a function"};
    }
};

template <class T>
inline std::shared_ptr<T> As(const std::shared_ptr<Object>& obj) {
    if (!Is<T>(obj)) {
        return nullptr;
    }
    return dynamic_pointer_cast<T>(obj);
}

template <class T>
inline bool Is(const std::shared_ptr<Object>& obj) {
    return dynamic_cast<T*>(obj.get()) != nullptr;
}

class Scope {
public:
    Scope() = default;
    Scope(Scope* anc_scope) : anc_scope_(anc_scope) {
    }
    Scope(const Scope& other) : anc_scope_(other.anc_scope_), vars_(other.vars_) {
    }
    Scope& operator=(Scope& other) {
        anc_scope_ = other.anc_scope_;
        vars_ = other.vars_;

        return *this;
    }

    Scope* CheckToSet(const std::string& name) {
        if (vars_.find(name) == vars_.end()) {
            Scope* anc = anc_scope_;
            while (true) {
                if (!anc) {
                    throw NameError{"no variable with name: " + name + " in all parent scopes"};
                }

                auto t = anc->vars_.find(name);
                if (t != anc->vars_.end()) {
                    return anc;
                }
                anc = anc->anc_scope_;
            }
        }

        return this;
    }

    std::shared_ptr<Object> At(const std::string& name) const {
        auto it = vars_.find(name);

        if (it == vars_.end()) {
            Scope* anc = anc_scope_;
            while (true) {
                if (!anc) {
                    throw NameError{"no variable with name: " + name + " in all parent scopes"};
                }

                auto t = anc->vars_.find(name);
                if (t != anc->vars_.end()) {
                    return t->second;
                }
                anc = anc->anc_scope_;
            }
        }

        return it->second;
    }

    void Assign(const std::string& name, const std::shared_ptr<Object>& value) {
        vars_[name] = value;
    }

    void Clear() {
        vars_.clear();
    }

    auto& GetVars() {
        return vars_;
    }

private:
    Scope* anc_scope_ = nullptr;
    std::map<std::string, std::shared_ptr<Object>> vars_{};
};

class Function : public Object {};

class Boolean : public Object {
public:
    Boolean(bool value) : value_(value) {
    }

    inline bool GetValue() const {
        return value_;
    }

    inline std::shared_ptr<Object> Eval(Scope&) override {
        return shared_from_this();
    }

    inline std::string Stringify() override {
        if (value_) {
            return "#t";
        }
        return "#f";
    }

private:
    bool value_;
};

class Number : public Object {
public:
    Number(int64_t value) : value_(value) {
    }

    inline int64_t GetValue() const {
        return value_;
    }

    inline std::shared_ptr<Object> Eval(Scope&) override {
        return shared_from_this();
    }

    inline std::string Stringify() override {
        return std::to_string(value_);
    }

private:
    int64_t value_;
};

class Symbol : public Object {
    static std::map<std::string, std::shared_ptr<Function>> k_functions;

public:
    Symbol(const std::string& value) : value_(value) {
    }
    Symbol(std::string&& value) : value_(std::move(value)) {
    }

    inline std::shared_ptr<Object> Eval(Scope& scope) override {
        if (auto it = k_functions.find(value_); it != k_functions.end()) {
            return it->second;
        }

        return scope.At(value_);
    }

    inline const std::string& GetName() const {
        return value_;
    }

    inline std::string Stringify() override {
        return value_;
    }

private:
    std::string value_;
};

class LambdaFunction : public Object {
    inline static std::vector<std::shared_ptr<Scope>> k_scopes{};

public:
    LambdaFunction() = default;
    LambdaFunction(Scope* anc_scope, const std::shared_ptr<Object>& body,
                   const std::vector<std::string>& args) {
        k_scopes.emplace_back(std::make_shared<Scope>(anc_scope));
        scope_ = k_scopes.back();
        body_ = body;
        args_ = args;
    }
    LambdaFunction(const LambdaFunction& other)
        : scope_(other.scope_), body_(other.body_), args_(other.args_) {
    }

    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>& head, Scope& scope) override;

    Scope& GetScope() {
        return *scope_;
    }
    const std::vector<std::string>& GetArgs() const {
        return args_;
    }
    std::shared_ptr<Object> GetBody() {
        return body_;
    }

private:
    std::shared_ptr<Scope> scope_ = nullptr;
    std::shared_ptr<Object> body_{};
    std::vector<std::string> args_{};
};

class Cell : public Object {
public:
    Cell(std::shared_ptr<Object> first, std::shared_ptr<Object> second)
        : first_(first), second_(second) {
    }

    Cell(const Cell& other) : first_(other.first_), second_(other.second_) {
    }

    Cell& operator=(const Cell& other) {
        first_ = other.first_;
        second_ = other.second_;

        return *this;
    }

    inline std::shared_ptr<Object> GetFirst() const {
        return first_;
    }
    inline std::shared_ptr<Object> GetSecond() const {
        return second_;
    }

    inline std::shared_ptr<Object> Eval(Scope& scope) override {
        if (!first_) {
            throw RuntimeError{"empty object in cell"};
        }
        auto eval = first_->Eval(scope);
        if (!eval) {
            throw RuntimeError{"apply on empty object in cell"};
        }
        if (Is<LambdaFunction>(eval) && Is<Cell>(second_) &&
            Is<Cell>(As<Cell>(second_)->GetFirst()) && As<Cell>(second_)->GetSecond() == nullptr) {
            auto new_args = As<Cell>(second_)->GetFirst()->Eval(scope);
            return eval->Apply(std::make_shared<Cell>(new_args, nullptr), scope);
        }
        return eval->Apply(second_, scope);
    }

    inline std::string Stringify() override {
        if (!first_) {
            return "(())";
        }

        std::vector<std::shared_ptr<Object>> list;
        std::shared_ptr<Object> curr = shared_from_this();
        auto proper = true;
        while (true) {
            auto first = As<Cell>(curr)->GetFirst();
            list.emplace_back(first);

            auto second = As<Cell>(curr)->GetSecond();
            if (second && !Is<Cell>(second)) {
                list.emplace_back(second);
                proper = false;
                break;
            }

            if (!second) {
                break;
            }

            curr = As<Cell>(curr)->GetSecond();
        }

        std::string res = "(";
        for (size_t i = 0; i + 1 < list.size(); ++i) {
            res += list[i]->Stringify();
            res += " ";
        }
        if (!proper && list.size() > 1) {
            res += ". ";
        }
        res += list.back()->Stringify();
        res += ")";

        return res;
    }

private:
    std::shared_ptr<Object> first_;
    std::shared_ptr<Object> second_;
};

size_t GetNumberOfArguments(const std::shared_ptr<Object>&);
template <class T>
std::vector<std::shared_ptr<Object>> MakeVector(const std::shared_ptr<Object>&, Scope&);

class ReturnItself : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

template <class T>
class IsType : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

using IsBoolean = IsType<Boolean>;
using IsNumber = IsType<Number>;
using IsSymbol = IsType<Symbol>;

class Not : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class Abs : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

template <typename F>
class CompareNumbers : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;

private:
    F f_{};
};

using Equal = CompareNumbers<std::equal_to<int64_t>>;
using Less = CompareNumbers<std::less<int64_t>>;
using Greater = CompareNumbers<std::greater<int64_t>>;
using LessEqual = CompareNumbers<std::less_equal<int64_t>>;
using GreaterEqual = CompareNumbers<std::greater_equal<int64_t>>;

template <typename F, int64_t init, bool has_one>
class AccumulateNumbers : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

template <class T>
struct MaxClass {
    inline T operator()(const T& lhs, const T& rhs) {
        return std::max<T>(lhs, rhs);
    }
};
template <class T>
struct MinClass {
    inline T operator()(const T& lhs, const T& rhs) {
        return std::min<T>(lhs, rhs);
    }
};

using Plus = AccumulateNumbers<std::plus<int64_t>, 0, true>;
using Prod = AccumulateNumbers<std::multiplies<int64_t>, 1, true>;
using Minus = AccumulateNumbers<std::minus<int64_t>, 0, false>;
using Divide = AccumulateNumbers<std::divides<int64_t>, 0, false>;
using Max = AccumulateNumbers<MaxClass<int64_t>, 0, false>;
using Min = AccumulateNumbers<MinClass<int64_t>, 0, false>;

class IsNull : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class IsPair : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class IsList : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class Cons : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class Car : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class Cdr : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class List : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class ListRef : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class ListTail : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

template <bool op>
class LogicOp : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

using And = LogicOp<true>;
using Or = LogicOp<false>;

class If : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class Define : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

class Set : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

template <bool car>
class SetPair : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};

using SetCar = SetPair<true>;
using SetCdr = SetPair<false>;

class CreateLambda : public Function {
public:
    std::shared_ptr<Object> Apply(const std::shared_ptr<Object>&, Scope&) override;
};