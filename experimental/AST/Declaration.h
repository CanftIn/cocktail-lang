#ifndef COCKTAIL_EXPERIMENTAL_AST_DECLARATION_H
#define COCKTAIL_EXPERIMENTAL_AST_DECLARATION_H

#include <list>
#include <string>

#include "experimental/AST/FunctionDefinition.h"
#include "experimental/AST/Member.h"
#include "experimental/AST/StructDefinition.h"

namespace Cocktail {

enum class DeclarationKind {
  FunctionDeclaration,
  StructDeclaration,
  ChoiceDeclaration
};

struct Declaration {
  DeclarationKind tag;
  union {
    struct FunctionDefinition* fun_def;

    struct StructDefinition* struct_def;

    struct {
      int line_num;
      std::string* name;
      std::list<std::pair<std::string, Expression*>>* alternatives;
    } choice_def;

  } u;
};

auto MakeFunDecl(struct FunctionDefinition* f) -> Declaration*;
auto MakeStructDecl(int line_num, std::string name, std::list<Member*>* members)
    -> Declaration*;
auto MakeChoiceDecl(int line_num, std::string name,
                    std::list<std::pair<std::string, Expression*>>* alts)
    -> Declaration*;

void PrintDecl(Declaration* d);

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_AST_DECLARATION_H