#ifndef COCKTAIL_EXPERIMENTAL_AST_STRUCT_DEFINITION_H
#define COCKTAIL_EXPERIMENTAL_AST_STRUCT_DEFINITION_H

#include <list>
#include <string>

#include "experimental/AST/Member.h"

namespace Cocktail {

struct StructDefinition {
  int line_num;
  std::string* name;
  std::list<Member*>* members;
};

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_AST_STRUCT_DEFINITION_H