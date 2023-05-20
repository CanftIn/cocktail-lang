#ifndef COCKTAIL_EXPERIMENTAL_AST_EXPRESSION_OR_FIELD_LIST_H
#define COCKTAIL_EXPERIMENTAL_AST_EXPRESSION_OR_FIELD_LIST_H

#include <list>

#include "experimental/AST/Expression.h"

namespace Cocktail {

enum class ExpOrFieldListKind { Exp, FieldList };

// This is used in the parsing of tuples and parenthesized expressions.
struct ExpOrFieldList {
  ExpOrFieldListKind tag;
  union {
    Expression* exp;
    std::list<std::pair<std::string, Expression*>>* fields;
  } u;
};

auto MakeExp(Expression* exp) -> ExpOrFieldList*;
auto MakeFieldList(std::list<std::pair<std::string, Expression*>>* fields)
    -> ExpOrFieldList*;
auto MakeConsField(ExpOrFieldList* e1, ExpOrFieldList* e2) -> ExpOrFieldList*;

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_AST_EXPRESSION_OR_FIELD_LIST_H