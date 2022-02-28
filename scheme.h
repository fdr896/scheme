#pragma once

#include "parser.h"
#include "object.h"
#include <sstream>

class Interpreter {
public:
    std::string Run(const std::string&);

private:
    Scope global_scope_{};
};