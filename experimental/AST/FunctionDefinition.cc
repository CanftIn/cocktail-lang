#include <iostream>

#include "experimental/AST/FunctionDefinition.h"

namespace Cocktail {

auto MakeFunDef(int line_num, std::string name, Expression* ret_type,
                Expression* param_pattern, Statement* body)
    -> struct FunctionDefinition* {
  auto* f = new struct FunctionDefinition();
  f->line_num = line_num;
  f->name = std::move(name);
  f->return_type = ret_type;
  f->param_pattern = param_pattern;
  f->body = body;
  return f;
}

void PrintFunDefDepth(struct FunctionDefinition* f, int depth) {
  std::cout << "fn " << f->name << " ";
  PrintExp(f->param_pattern);
  std::cout << " -> ";
  PrintExp(f->return_type);
  if (f->body) {
    std::cout << " {" << std::endl;
    PrintStatement(f->body, depth);
    std::cout << std::endl << "}" << std::endl;
  } else {
    std::cout << ";" << std::endl;
  }
}

void PrintFunDef(struct FunctionDefinition* f) { PrintFunDefDepth(f, -1); }

}  // namespace Cocktail