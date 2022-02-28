#pragma once

#include <variant>
#include <cctype>
#include <optional>
#include <istream>
#include "error.h"

struct SymbolToken {
    std::string name;

    bool operator==(const SymbolToken& other) const;
};

struct QuoteToken {
    bool operator==(const QuoteToken&) const;
};

struct DotToken {
    bool operator==(const DotToken&) const;
};

enum class BracketToken { OPEN, CLOSE };

struct ConstantToken {
    int64_t value;

    bool operator==(const ConstantToken& other) const;
};

struct BooleanToken {
    bool value;

    bool operator==(const BooleanToken& other) const;
};

using Token =
    std::variant<ConstantToken, BracketToken, SymbolToken, QuoteToken, DotToken, BooleanToken>;

class Tokenizer {
public:
    Tokenizer(std::istream* in) : in_(in) {
        size_t len = 0;
        while (!IsEnd()) {
            char curr = in_->get();
            if (!AvailableChars(curr)) {
                throw SyntaxError{"unavailable symbol: " + std::string(1, curr) +
                                  std::to_string(int(curr))};
            }
            len += 1;
        }
        while (len > 0) {
            in_->unget();
            len -= 1;
        }

        SkipSpaces();
    }

    bool IsEnd();

    void Next();

    Token GetToken();

private:
    void SkipSpaces() {
        while (!IsEnd() && std::isspace(in_->peek())) {
            in_->get();
        }
    }

    static bool BeginsWith(char c) {
        static const std::string kAvailable = "<=>*#/";
        return std::isalpha(c) || kAvailable.find(c) != std::string::npos;
    };
    static bool AvailableCharsInSymbol(char c) {
        static const std::string kAvailable = "?!-";
        return BeginsWith(c) || std::isdigit(c) || kAvailable.find(c) != std::string::npos;
    };
    static bool AvailableChars(char c) {
        static const std::string kAvailable = "().'+-";
        return AvailableCharsInSymbol(c) || std::isspace(c) ||
               kAvailable.find(c) != std::string::npos;
    }

    std::istream* in_;
    size_t token_len_{};
};