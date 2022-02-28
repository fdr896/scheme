#include "parser.h"
#include <vector>
#include <iostream>

std::shared_ptr<Object> Read(Tokenizer* tokenizer) {
    if (tokenizer->IsEnd()) {
        throw SyntaxError{"in Read: empty list"};
    }

    Token token;
    token = tokenizer->GetToken();
    tokenizer->Next();

    if (QuoteToken* _ = std::get_if<QuoteToken>(&token)) {
        if (tokenizer->IsEnd()) {
            throw SyntaxError{"there should be something after quote"};
        }
        Token next_token = tokenizer->GetToken();
        if (BracketToken* x = std::get_if<BracketToken>(&next_token)) {
            if (*x != BracketToken::OPEN) {
                throw SyntaxError{"there can not be ) after quote"};
            }
            tokenizer->Next();

            auto list = ReadList(tokenizer);
            if (!list) {
                return std::make_shared<Cell>(std::make_shared<Symbol>("quote"), nullptr);
            }
            return std::make_shared<Cell>(std::make_shared<Symbol>("quote"),
                                          std::make_shared<Cell>(list, nullptr));
        } else {
            return std::make_shared<Cell>(std::make_shared<Symbol>("quote"), Read(tokenizer));
        }
    } else if (ConstantToken* x = std::get_if<ConstantToken>(&token)) {
        return std::make_shared<Number>(x->value);
    } else if (BooleanToken* x = std::get_if<BooleanToken>(&token)) {
        return std::make_shared<Boolean>(x->value);
    } else if (SymbolToken* x = std::get_if<SymbolToken>(&token)) {
        return std::make_shared<Symbol>(x->name);
    } else if (BracketToken* x = std::get_if<BracketToken>(&token)) {
        if (*x != BracketToken::OPEN) {
            throw SyntaxError{"in Read: expected ("};
        }

        return ReadList(tokenizer);
    }

    throw SyntaxError{"in Read: bad pattern"};
}

std::shared_ptr<Object> ReadList(Tokenizer* tokenizer) {
    std::vector<std::shared_ptr<Object>> list;

    size_t number_of_dots = 0;
    size_t dot_pos;
    size_t curr_pos = 0;
    while (true) {
        if (tokenizer->IsEnd()) {
            throw SyntaxError{"in ReadList: expected )"};
        }

        Token token = tokenizer->GetToken();

        if (BracketToken* x = std::get_if<BracketToken>(&token)) {
            if (*x == BracketToken::CLOSE) {
                tokenizer->Next();
                break;
            }
        }
        if (DotToken* _ = std::get_if<DotToken>(&token)) {
            number_of_dots += 1;
            if (number_of_dots > 1) {
                throw SyntaxError{"in ReadList: incorrect list"};
            }
            dot_pos = curr_pos;
            curr_pos += 1;
            tokenizer->Next();
            list.emplace_back(nullptr);
            continue;
        }

        list.emplace_back(Read(tokenizer));
        curr_pos += 1;
    }

    size_t sz = list.size();
    if (number_of_dots == 0) {
        if (list.empty()) {
            return nullptr;
        }

        std::shared_ptr<Object> cell = std::make_shared<Cell>(list[sz - 1], nullptr);
        for (int i = sz - 2; i >= 0; --i) {
            cell = std::make_shared<Cell>(list[i], cell);
        }

        if (auto symb = As<Symbol>(list.front())) {
            if (symb->GetName() == "if" && !(sz == 3 || sz == 4)) {
                throw SyntaxError{"if should have condition and 1 or 2 statements"};
            }
            if (symb->GetName() == "define" || symb->GetName() == "set!") {
                if (sz == 4) {
                    if (!Is<Cell>(list[1])) {
                        throw SyntaxError{symb->GetName() +
                                          " with more than 2 arguments should define lambda"};
                    }
                    return cell;
                }
                if (sz != 3) {
                    throw SyntaxError{symb->GetName() + " should have 2 arguments"};
                }
            }
            if (symb->GetName() == "lambda") {
                if (sz < 3) {
                    throw SyntaxError{symb->GetName() + " should have at least 2 arguments"};
                }
            }
        }

        return cell;
    } else {
        if (dot_pos + 2 != sz) {
            throw SyntaxError{"in ReadList: incorrect improper list with dot_pos = " +
                              std::to_string(dot_pos) + " and sz = " + std::to_string(sz)};
        }

        if (sz < 3) {
            throw SyntaxError{"in ReadList: incorrect improper list with dot_pos = " +
                              std::to_string(dot_pos) + " and sz = " + std::to_string(sz)};
        }

        std::shared_ptr<Object> cell = std::make_shared<Cell>(list[sz - 3], list[sz - 1]);
        if (sz > 3) {
            for (int i = sz - 4; i >= 0; --i) {
                cell = std::make_shared<Cell>(list[i], cell);
            }
        }

        return cell;
    }
}