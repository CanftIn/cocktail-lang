#include <cstdio>
#include <cstring>
#include <iostream>

#include "experimental/SyntaxHelper.h"

extern FILE* yyin;
extern auto yyparse() -> int;  // NOLINT(readability-identifier-naming)

int main(int argc, char* argv[]) {
  // yydebug = 1;

  if (argc > 1) {
    Cocktail::input_filename = argv[1];
    yyin = fopen(argv[1], "r");
    if (yyin == nullptr) {
      std::cerr << "Error opening '" << argv[1] << "': " << strerror(errno)
                << std::endl;
      return 1;
    }
  }
  return yyparse();
}