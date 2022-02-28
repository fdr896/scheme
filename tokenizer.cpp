#include "tokenizer.h"

bool SymbolToken::operator==(const SymbolToken& other) const {
    return name == other.name;
}

bool QuoteToken::operator==(const QuoteToken&) const {
    return true;
}

bool DotToken::operator==(const DotToken&) const {
    return true;
}

bool ConstantToken::operator==(const ConstantToken& other) const {
    return value == other.value;
}

bool BooleanToken::operator==(const BooleanToken& other) const {
    return value == other.value;
}

bool Tokenizer::IsEnd() {
    return in_->eof() || in_->peek() == EOF;
}

void Tokenizer::Next() {
    auto token = GetToken();

    if (SymbolToken* x = std::get_if<SymbolToken>(&token)) {
        token_len_ = (x->name).size();
    }

    while (token_len_ > 0) {
        in_->get();
        token_len_ -= 1;
    }
    SkipSpaces();
}

Token Tokenizer::GetToken() {
    token_len_ = 0;
    char curr = in_->get();

    if (curr == '\'') {
        token_len_ = 1;
        in_->unget();
        return Token{QuoteToken{}};
    }
    if (curr == '.') {
        token_len_ = 1;
        in_->unget();
        return Token{DotToken{}};
    }
    if (curr == '#') {
        if (!IsEnd() && (in_->peek() == 't' || in_->peek() == 'f')) {
            char val = in_->peek();
            in_->unget();
            token_len_ = 2;

            if (val == 't') {
                return Token{BooleanToken{true}};
            }
            return Token{BooleanToken{false}};
        }
    }
    if (curr == '(' || curr == ')') {
        token_len_ = 1;
        in_->unget();
        if (curr == '(') {
            return Token{BracketToken::OPEN};
        }
        return Token{BracketToken::CLOSE};
    }

    size_t len = 1;

    bool is_digit = std::isdigit(curr);
    if ((curr == '+' || curr == '-') && !IsEnd() && std::isdigit(in_->peek())) {
        is_digit = true;
    }
    if (is_digit) {
        char sgn = 1;
        if (curr == '-') {
            sgn = -1;
        } else if (curr == '+') {
            token_len_ = 1;
        }

        int64_t curr_digit = 0;
        if (curr != '+' && curr != '-') {
            curr_digit = static_cast<int64_t>(curr - '0');
        }
        while (!IsEnd() && std::isdigit(in_->peek())) {
            len += 1;
            curr = in_->get();
            curr_digit = curr_digit * 10 + static_cast<int64_t>(curr - '0');
        }
        curr_digit *= sgn;

        while (len > 0) {
            in_->unget();
            len -= 1;
        }
        token_len_ += std::to_string(curr_digit).size();
        return Token{ConstantToken{curr_digit}};
    }

    if (curr == '+' || curr == '-') {
        in_->unget();
        return Token{SymbolToken{std::string(1, curr)}};
    }

    if (!BeginsWith(curr)) {
        throw SyntaxError{"syntax error:" + std::string(1, curr) + std::to_string(int(curr))};
    }
    std::string curr_symbol(1, curr);
    while (!IsEnd() && AvailableCharsInSymbol(in_->peek())) {
        len += 1;
        curr_symbol.push_back(in_->get());
    }

    while (len > 0) {
        in_->unget();
        len -= 1;
    }

    return Token{SymbolToken{curr_symbol}};
}