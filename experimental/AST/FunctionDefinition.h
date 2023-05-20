#ifndef COCKTAIL_EXPERIMENTAL_AST_FUNCTION_DEFINITION_H
#define COCKTAIL_EXPERIMENTAL_AST_FUNCTION_DEFINITION_H

#include "experimental/AST/Expression.h"
#include "experimental/AST/Statement.h"

namespace Cocktail {

struct FunctionDefinition {
  int line_num;
  std::string name;
  Expression* param_pattern;
  Expression* return_type;
  Statement* body;
};

auto MakeFunDef(int line_num, std::string name, Expression* ret_type,
                Expression* param, Statement* body)
    -> struct FunctionDefinition*;
void PrintFunDef(struct FunctionDefinition*);
void PrintFunDefDepth(struct FunctionDefinition*, int);

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_AST_FUNCTION_DEFINITION_H