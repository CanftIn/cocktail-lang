#include "experimental/AST/Member.h"

#include <iostream>

namespace Cocktail {

auto MakeField(int line_num, std::string name, Expression* type) -> Member* {
  auto m = new Member();
  m->line_num = line_num;
  m->tag = MemberKind::FieldMember;
  m->u.field.name = new std::string(std::move(name));
  m->u.field.type = type;
  return m;
}

void PrintMember(Member* m) {
  switch (m->tag) {
    case MemberKind::FieldMember:
      std::cout << "var " << *m->u.field.name << " : ";
      PrintExp(m->u.field.type);
      std::cout << ";" << std::endl;
      break;
  }
}

}  // namespace Cocktail