#include "object.h"
#include <random>

size_t GetNumberOfArguments(const std::shared_ptr<Object>& head) {
    if (!head) {
        return 0;
    }

    auto cell = As<Cell>(head);
    if (cell) {
        return 1 + GetNumberOfArguments(cell->GetSecond());
    }
    return 1;
}

template <class T>
std::vector<std::shared_ptr<Object>> MakeVector(const std::shared_ptr<Object>& head, Scope& scope) {
    if (!head) {
        return {};
    }

    auto curr = head;
    if (!Is<Cell>(curr) || !As<Cell>(curr)->GetFirst()) {
        throw RuntimeError{"MakeVector arguments should have type T"};
    }
    std::vector<std::shared_ptr<Object>> result;
    while (curr) {
        auto t = As<T>(As<Cell>(curr)->GetFirst()->Eval(scope));
        if (!t) {
            throw RuntimeError{"MakeVector arguments should have type T"};
        }
        result.emplace_back(t);
        auto cell = As<Cell>(curr);
        if (!cell) {
            throw RuntimeError{"MakeVector deals only with lists"};
        }
        curr = cell->GetSecond();

        if (curr && !Is<Cell>(curr)) {
            if (Is<T>(curr)) {
                result.emplace_back(curr);
            }

            throw RuntimeError{"MakeVector arguments should have type T"};
        }
    }

    return result;
}

std::shared_ptr<Object> ReturnItself::Apply(const std::shared_ptr<Object>& head, Scope&) {
    if (Is<Cell>(head) && As<Cell>(head)->GetSecond() == nullptr) {
        return As<Cell>(head)->GetFirst();
    }
    return head;
}

template <class T>
std::shared_ptr<Object> IsType<T>::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    if (GetNumberOfArguments(head) != 1) {
        throw RuntimeError{"IsType expects 1 argument"};
    }
    if (!Is<Cell>(head)) {
        throw RuntimeError{"IsType expects cell with 1 argument"};
    }

    if (Is<T>(As<Cell>(head)->GetFirst()->Eval(scope))) {
        return std::make_shared<Boolean>(true);
    }
    return std::make_shared<Boolean>(false);
}

std::shared_ptr<Object> Not::Apply(const std::shared_ptr<Object>& head, Scope&) {
    if (GetNumberOfArguments(head) != 1) {
        throw RuntimeError{"Not expects 1 argument"};
    }
    if (!Is<Cell>(head)) {
        throw RuntimeError{"Not expects cell with 1 argument"};
    }
    auto val = As<Boolean>(As<Cell>(head)->GetFirst());
    if (!val) {
        return std::make_shared<Boolean>(false);
    }

    if (val->GetValue()) {
        return std::make_shared<Boolean>(false);
    }
    return std::make_shared<Boolean>(true);
}

std::shared_ptr<Object> Abs::Apply(const std::shared_ptr<Object>& head, Scope&) {
    if (GetNumberOfArguments(head) != 1) {
        throw RuntimeError{"Not expects 1 argument"};
    }
    if (!Is<Cell>(head)) {
        throw RuntimeError{"Not expects cell with 1 argument"};
    }
    auto val = As<Number>(As<Cell>(head)->GetFirst());
    if (!val) {
        throw RuntimeError{"Abs expects a Number as an argument"};
    }

    return std::make_shared<Number>(std::abs(val->GetValue()));
}

template <typename F>
std::shared_ptr<Object> CompareNumbers<F>::Apply(const std::shared_ptr<Object>& head,
                                                 Scope& scope) {
    auto list = MakeVector<Number>(head, scope);

    F cmp{};
    for (size_t i = 0; i + 1 < list.size(); ++i) {
        if (!cmp(As<Number>(list[i])->GetValue(), As<Number>(list[i + 1])->GetValue())) {
            return std::make_shared<Boolean>(false);
        }
    }

    return std::make_shared<Boolean>(true);
}

template <typename F, int64_t init, bool has_one>
std::shared_ptr<Object> AccumulateNumbers<F, init, has_one>::Apply(
    const std::shared_ptr<Object>& head, Scope& scope) {
    auto list = MakeVector<Number>(head, scope);

    if (list.empty()) {
        if (has_one) {
            return std::make_shared<Number>(init);
        }
        throw RuntimeError{"AccumulateNumbers invokes op without one element"};
    }

    F op{};
    int64_t res = As<Number>(list.front())->GetValue();
    for (size_t i = 1; i < list.size(); ++i) {
        res = op(res, As<Number>(list[i])->GetValue());
    }

    return std::make_shared<Number>(res);
}

std::shared_ptr<Object> IsPair::Apply(const std::shared_ptr<Object>& head_, Scope& scope) {
    auto head = As<Cell>(head_)->GetFirst();
    head = head->Eval(scope);

    auto cell = As<Cell>(head);
    if (!cell) {
        return std::make_shared<Boolean>(false);
    }

    auto second = As<Cell>(cell->GetSecond());
    if (second) {
        if (second->GetSecond()) {
            return std::make_shared<Boolean>(false);
        }

        return std::make_shared<Boolean>(true);
    }

    return std::make_shared<Boolean>(Is<Number>(cell->GetSecond()));
}

std::shared_ptr<Object> IsNull::Apply(const std::shared_ptr<Object>& head_, Scope& scope) {
    auto head = As<Cell>(head_)->GetFirst();
    head = head->Eval(scope);
    return std::make_shared<Boolean>(!head);
}

std::shared_ptr<Object> IsList::Apply(const std::shared_ptr<Object>& head_, Scope& scope) {
    auto head = As<Cell>(head_)->GetFirst();
    head = head->Eval(scope);
    if (!head) {
        return std::make_shared<Boolean>(true);
    }
    auto cell = As<Cell>(head);
    while (Is<Cell>(cell->GetSecond())) {
        cell = As<Cell>(cell->GetSecond());
    }

    if (Is<Number>(cell->GetSecond())) {
        return std::make_shared<Boolean>(false);
    }
    return std::make_shared<Boolean>(true);
}

std::shared_ptr<Object> Cons::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    if (GetNumberOfArguments(head) != 2) {
        throw RuntimeError{"Cons requires 2 arguments"};
    }

    auto cell = As<Cell>(head);
    auto first = cell->GetFirst()->Eval(scope);
    auto second = As<Cell>(cell->GetSecond())->GetFirst()->Eval(scope);

    return std::make_shared<Cell>(first, second);
}

std::shared_ptr<Object> Car::Apply(const std::shared_ptr<Object>& head_, Scope& scope) {
    auto head = As<Cell>(head_)->GetFirst();
    head = head->Eval(scope);
    if (!head) {
        throw RuntimeError{"Car requires not empty cell as argument"};
    }
    auto cell = As<Cell>(head);
    if (!cell) {
        cell = As<Cell>(head->Eval(scope));
    }

    return As<Cell>(cell)->GetFirst();
}

std::shared_ptr<Object> Cdr::Apply(const std::shared_ptr<Object>& head_, Scope& scope) {
    auto head = As<Cell>(head_)->GetFirst();
    head = head->Eval(scope);
    if (!head) {
        throw RuntimeError{"Cdr requires not empty cell as argument"};
    }
    auto cell = As<Cell>(head);
    while (!cell) {
        cell = As<Cell>(head->Eval(scope));
    }

    return As<Cell>(cell)->GetSecond();
}

std::shared_ptr<Object> List::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    if (!head) {
        return nullptr;
    }
    auto list = MakeVector<Number>(head, scope);
    int sz = list.size();
    std::shared_ptr<Object> cell = std::make_shared<Cell>(list[sz - 1], nullptr);
    for (int i = sz - 2; i >= 0; --i) {
        cell = std::make_shared<Cell>(list[i], cell);
    }

    return cell;
}

std::shared_ptr<Object> ListRef::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    auto cell = As<Cell>(head);
    if (!cell) {
        throw RuntimeError{"list-ref expected 2 arguments"};
    }

    auto first = cell->GetFirst()->Eval(scope);
    auto elems = MakeVector<Number>(first, scope);
    auto second = As<Number>(As<Cell>(cell->GetSecond())->GetFirst());
    if (!second) {
        throw RuntimeError{"list-ref expected Number as 2 argument"};
    }

    if (static_cast<size_t>(second->GetValue()) >= elems.size()) {
        throw RuntimeError{"list-ref: index out of range"};
    }

    return std::make_shared<Number>(As<Number>(elems[second->GetValue()])->GetValue());
}

std::shared_ptr<Object> ListTail::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    auto cell = As<Cell>(head);
    if (!cell) {
        throw RuntimeError{"list-ref expected 2 arguments"};
    }

    auto first = cell->GetFirst()->Eval(scope);
    auto elems = MakeVector<Number>(first, scope);
    auto second = As<Number>(As<Cell>(cell->GetSecond())->GetFirst());
    if (!second) {
        throw RuntimeError{"list-ref expected Number as 2 argument"};
    }

    size_t idx = second->GetValue();

    if (idx > elems.size()) {
        throw RuntimeError{"list-ref: index out of range"};
    }
    if (idx == elems.size()) {
        return nullptr;
    }

    int sz = elems.size();
    std::shared_ptr<Object> list = std::make_shared<Cell>(elems[sz - 1], nullptr);
    for (size_t i = static_cast<int>(sz) - 2; i >= idx; --i) {
        list = std::make_shared<Cell>(elems[i], list);
    }

    return list;
}

template <bool op>
std::shared_ptr<Object> LogicOp<op>::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    auto arg = head;
    std::shared_ptr<Object> last;
    if (!head) {
        return std::make_shared<Boolean>(op);
    }
    auto args = GetNumberOfArguments(head);
    while (args--) {
        auto val = As<Cell>(arg)->GetFirst()->Eval(scope);

        if (Is<Boolean>(val)) {
            auto curr = As<Boolean>(val)->GetValue();
            if (op) {
                curr = !curr;
            }
            if (curr) {
                return val;
            }
        }

        last = val;
        arg = As<Cell>(arg)->GetSecond();
    }

    return last;
}

std::shared_ptr<Object> If::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    auto cell = As<Cell>(head);
    auto cond = cell->GetFirst()->Eval(scope);
    if (!Is<Boolean>(cond)) {
        throw RuntimeError{"If condition must can be evaluated into Boolean"};
    }

    if (As<Boolean>(cond)->GetValue()) {
        return As<Cell>(cell->GetSecond())->GetFirst()->Eval(scope);
    }

    auto branches = As<Cell>(cell->GetSecond());
    auto second_branch = branches->GetSecond();
    if (As<Cell>(branches)->GetSecond() != nullptr) {
        return As<Cell>(second_branch)->GetFirst()->Eval(scope);
    }

    return nullptr;
}

std::shared_ptr<Object> Define::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    auto cell = As<Cell>(head);
    auto name = As<Symbol>(cell->GetFirst());
    if (!name) {
        auto lambda = As<Cell>(cell->GetFirst());
        if (!lambda) {
            throw RuntimeError{"Define should define a symbol or lambda"};
        }

        auto curr_name = As<Symbol>(lambda->GetFirst())->GetName();
        std::vector<std::string> args;
        auto curr_args = lambda->GetSecond();
        while (curr_args) {
            args.emplace_back(As<Symbol>(As<Cell>(curr_args)->GetFirst())->GetName());
            curr_args = As<Cell>(curr_args)->GetSecond();
        }

        auto body = As<Cell>(cell->GetSecond());
        auto new_lambda = std::make_shared<LambdaFunction>(&scope, body, args);
        scope.Assign(curr_name, new_lambda);

        return nullptr;
    }

    auto value = As<Cell>(cell->GetSecond())->GetFirst()->Eval(scope);

    scope.Assign(name->GetName(), value);

    return nullptr;
}

std::shared_ptr<Object> Set::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    auto cell = As<Cell>(head);
    auto name = As<Symbol>(cell->GetFirst());
    if (!name) {
        throw RuntimeError{"Set should define a symbol"};
    }
    Scope* to_assign = scope.CheckToSet(name->GetName());
    auto value = As<Cell>(cell->GetSecond())->GetFirst()->Eval(scope);

    to_assign->Assign(name->GetName(), value);

    return nullptr;
}

template <bool car>
std::shared_ptr<Object> SetPair<car>::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    auto cell = As<Cell>(head);
    auto name = As<Symbol>(cell->GetFirst());
    if (!name) {
        name = As<Symbol>(cell->GetFirst()->Eval(scope));
    }
    scope.CheckToSet(name->GetName());

    auto new_value = As<Cell>(cell->GetSecond())->GetFirst();
    if (Is<Cell>(new_value)) {
        new_value = new_value->Eval(scope);
    }

    if constexpr (car) {
        auto old_second = As<Cell>(scope.At(name->GetName()))->GetSecond();

        scope.Assign(name->GetName(), std::make_shared<Cell>(new_value, old_second));
    } else {
        auto old_first = As<Cell>(scope.At(name->GetName()))->GetFirst();

        scope.Assign(name->GetName(), std::make_shared<Cell>(old_first, new_value));
    }

    return nullptr;
}

std::shared_ptr<Object> CreateLambda::Apply(const std::shared_ptr<Object>& head, Scope& scope) {
    static auto gen_lambda_name = []() -> std::string {
        static std::mt19937 rnd(time(0));
        static std::uniform_int_distribution<int> dist(0, 25);
        size_t len = 10;
        std::string name;
        for (size_t i = 0; i < len; ++i) {
            name.push_back(static_cast<char>('a' + dist(rnd)));
        }

        return name;
    };

    auto curr_name = gen_lambda_name();
    auto cell = As<Cell>(head);
    std::vector<std::string> args;
    auto curr_args = cell->GetFirst();
    while (curr_args) {
        args.emplace_back(As<Symbol>(As<Cell>(curr_args)->GetFirst())->GetName());
        curr_args = As<Cell>(curr_args)->GetSecond();
    }

    auto body = As<Cell>(cell->GetSecond());
    auto lambda = std::make_shared<LambdaFunction>(&scope, body, args);
    scope.Assign(curr_name, lambda);

    return lambda;
}

std::shared_ptr<Object> LambdaFunction::Apply(const std::shared_ptr<Object>& head, Scope&) {
    auto& scope = GetScope();
    auto args = GetArgs();
    auto body = As<Cell>(GetBody());

    auto cell = As<Cell>(head);
    for (size_t i = 0; i < args.size(); ++i) {
        auto arg = cell->GetFirst()->Eval(scope);
        scope.Assign(args[i], arg);
        cell = As<Cell>(cell->GetSecond());
    }

    std::shared_ptr<Object> res;
    LambdaFunction::k_scopes.emplace_back(std::make_shared<Scope>(&scope));
    auto& new_scope = LambdaFunction::k_scopes.back();
    new_scope->GetVars() = scope.GetVars();
    while (body) {
        res = body->GetFirst()->Eval(*new_scope);
        body = As<Cell>(body->GetSecond());
    }

    return res;
}

std::map<std::string, std::shared_ptr<Function>> Symbol::k_functions =
    std::map<std::string, std::shared_ptr<Function>>{
        {"quote", std::make_shared<ReturnItself>()},
        {"boolean?", std::make_shared<IsBoolean>()},
        {"number?", std::make_shared<IsNumber>()},
        {"symbol?", std::make_shared<IsSymbol>()},
        {"not", std::make_shared<Not>()},
        {"abs", std::make_shared<Abs>()},
        {"=", std::make_shared<Equal>()},
        {"<", std::make_shared<Less>()},
        {">", std::make_shared<Greater>()},
        {"<=", std::make_shared<LessEqual>()},
        {">=", std::make_shared<GreaterEqual>()},
        {"+", std::make_shared<Plus>()},
        {"*", std::make_shared<Prod>()},
        {"-", std::make_shared<Minus>()},
        {"/", std::make_shared<Divide>()},
        {"max", std::make_shared<Max>()},
        {"min", std::make_shared<Min>()},
        {"pair?", std::make_shared<IsPair>()},
        {"null?", std::make_shared<IsNull>()},
        {"list?", std::make_shared<IsList>()},
        {"cons", std::make_shared<Cons>()},
        {"car", std::make_shared<Car>()},
        {"cdr", std::make_shared<Cdr>()},
        {"list", std::make_shared<List>()},
        {"list-ref", std::make_shared<ListRef>()},
        {"list-tail", std::make_shared<ListTail>()},
        {"and", std::make_shared<And>()},
        {"or", std::make_shared<Or>()},
        {"if", std::make_shared<If>()},
        {"define", std::make_shared<Define>()},
        {"set!", std::make_shared<Set>()},
        {"set-car!", std::make_shared<SetCar>()},
        {"set-cdr!", std::make_shared<SetCdr>()},
        {"lambda", std::make_shared<CreateLambda>()},
    };