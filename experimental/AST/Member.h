#ifndef COCKTAIL_EXPERIMENTAL_AST_MEMBER_H
#define COCKTAIL_EXPERIMENTAL_AST_MEMBER_H

#include <string>

#include "experimental/AST/Expression.h"

namespace Cocktail {

enum class MemberKind {
  FieldMember,
};

struct Member {
  int line_num;
  MemberKind tag;
  union {
    struct {
      std::string* name;
      Expression* type;
    } field;
  } u;
};

auto MakeField(int line_num, std::string name, Expression* type) -> Member*;

void PrintMember(Member* member);

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_AST_MEMBER_H