#include "scheme.h"

std::string Interpreter::Run(const std::string& code) {
    std::stringstream code_stream(code);
    Tokenizer tokenizer(&code_stream);

    auto result = Read(&tokenizer);
    if (!tokenizer.IsEnd()) {
        throw SyntaxError{"code can not be parsed"};
    }
    if (!result) {
        throw RuntimeError{"null expression can not be evaluated"};
    }

    auto to_string = result->Eval(global_scope_);
    if (!to_string) {
        return "()";
    }
    return to_string->Stringify();
}