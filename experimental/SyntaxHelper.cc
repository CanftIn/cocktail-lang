#include "experimental/SyntaxHelper.h"

#include <iostream>

#include "experimental/Interpreter/Interpreter.h"
#include "experimental/Interpreter/TypeCheck.h"

namespace Cocktail {

char* input_filename = nullptr;

void PrintSyntaxError(char* error, int line_num) {
  std::cerr << input_filename << ":" << line_num << ": " << error << std::endl;
  exit(-1);
}

void ExecProgram(std::list<Declaration*>* fs) {
  std::cout << "********** source program **********" << std::endl;
  for (const auto& decl : *fs) {
    PrintDecl(decl);
  }
  std::cout << "********** type checking **********" << std::endl;
  state = new State();  // Compile-time state.
  std::pair<TypeEnv*, Env*> p = TopLevel(fs);
  TypeEnv* top = p.first;
  Env* ct_top = p.second;
  std::list<Declaration*> new_decls;
  for (const auto& i : *fs) {
    new_decls.push_back(TypeCheckDecl(i, top, ct_top));
  }
  std::cout << std::endl;
  std::cout << "********** type checking complete **********" << std::endl;
  for (const auto& decl : new_decls) {
    PrintDecl(decl);
  }
  std::cout << "********** starting execution **********" << std::endl;
  int result = InterpProgram(&new_decls);
  std::cout << "result: " << result << std::endl;
}

}  // namespace Cocktail