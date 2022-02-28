#include <iostream>

#include "scheme.h"

int main() {
  Interpreter interpreter;

  std::cout << interpreter.Run("(+ 1 2)") << "\n";
  std::cout << interpreter.Run("((lambda (x) (+ 1 x)) 5)") << "\n";

  return 0;
}