#ifndef COCKTAIL_EXPERIMENTAL_INTERPRETER_ACTION_H
#define COCKTAIL_EXPERIMENTAL_INTERPRETER_ACTION_H

#include <iostream>
#include <vector>

#include "experimental/AST/Expression.h"
#include "experimental/AST/Statement.h"
#include "experimental/Interpreter/ConsList.h"
#include "experimental/Interpreter/Value.h"

namespace Cocktail {

enum class ActionKind {
  LValAction,
  ExpressionAction,
  StatementAction,
  ValAction,
  ExpToLValAction,
  DeleteTmpAction
};

struct Action {
  ActionKind tag;
  union {
    Expression* exp;  // for LValAction and ExpressionAction
    Statement* stmt;
    Value* val;  // for finished actions with a value (ValAction)
    Address delete_tmp;
  } u;
  int pos;                      // position or state of the action
  std::vector<Value*> results;  // results from subexpression
};

void PrintAct(Action* act, std::ostream& out);
void PrintActList(Cons<Action*>* ls, std::ostream& out);
auto MakeExpAct(Expression* e) -> Action*;
auto MakeLvalAct(Expression* e) -> Action*;
auto MakeStmtAct(Statement* s) -> Action*;
auto MakeValAct(Value* v) -> Action*;
auto MakeExpToLvalAct() -> Action*;
auto MakeDeleteAct(Address a) -> Action*;

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_INTERPRETER_ACTION_H