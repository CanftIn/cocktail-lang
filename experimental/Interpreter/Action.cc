#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "experimental/AST/Expression.h"
#include "experimental/AST/FunctionDefinition.h"
#include "experimental/Interpreter/Interpreter.h"
#include "experimental/Interpreter/TypeCheck.h"

namespace Cocktail {

void PrintAct(Action* act, std::ostream& out) {
  switch (act->tag) {
    case ActionKind::DeleteTmpAction:
      std::cout << "delete_tmp(" << act->u.delete_tmp << ")";
      break;
    case ActionKind::ExpToLValAction:
      out << "exp=>lval";
      break;
    case ActionKind::LValAction:
    case ActionKind::ExpressionAction:
      PrintExp(act->u.exp);
      break;
    case ActionKind::StatementAction:
      PrintStatement(act->u.stmt, 1);
      break;
    case ActionKind::ValAction:
      PrintValue(act->u.val, out);
      break;
  }
  out << "<" << act->pos << ">";
  if (act->results.size() > 0) {
    out << "(";
    for (auto& result : act->results) {
      if (result) {
        PrintValue(result, out);
      }
      out << ",";
    }
    out << ")";
  }
}

void PrintActList(Cons<Action*>* ls, std::ostream& out) {
  if (ls) {
    PrintAct(ls->curr, out);
    if (ls->next) {
      out << " :: ";
      PrintActList(ls->next, out);
    }
  }
}

auto MakeExpAct(Expression* e) -> Action* {
  auto* act = new Action();
  act->tag = ActionKind::ExpressionAction;
  act->u.exp = e;
  act->pos = -1;
  return act;
}

auto MakeLvalAct(Expression* e) -> Action* {
  auto* act = new Action();
  act->tag = ActionKind::LValAction;
  act->u.exp = e;
  act->pos = -1;
  return act;
}

auto MakeStmtAct(Statement* s) -> Action* {
  auto* act = new Action();
  act->tag = ActionKind::StatementAction;
  act->u.stmt = s;
  act->pos = -1;
  return act;
}

auto MakeValAct(Value* v) -> Action* {
  auto* act = new Action();
  act->tag = ActionKind::ValAction;
  act->u.val = v;
  act->pos = -1;
  return act;
}

auto MakeExpToLvalAct() -> Action* {
  auto* act = new Action();
  act->tag = ActionKind::ExpToLValAction;
  act->pos = -1;
  return act;
}

auto MakeDeleteAct(Address a) -> Action* {
  auto* act = new Action();
  act->tag = ActionKind::DeleteTmpAction;
  act->pos = -1;
  act->u.delete_tmp = a;
  return act;
}

}  // namespace Cocktail