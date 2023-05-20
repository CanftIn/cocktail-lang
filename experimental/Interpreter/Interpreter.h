#ifndef COCKTAIL_EXPERIMENTAL_INTERPRETER_INTERPRETER_H
#define COCKTAIL_EXPERIMENTAL_INTERPRETER_INTERPRETER_H

#include <list>
#include <utility>
#include <vector>

#include "experimental/AST/Declaration.h"
#include "experimental/Interpreter/Action.h"
#include "experimental/Interpreter/AssocList.h"
#include "experimental/Interpreter/ConsList.h"
#include "experimental/Interpreter/Value.h"

namespace Cocktail {

using Env = AssocList<std::string, Address>;

/***** Scopes *****/

struct Scope {
  Scope(Env* e, std::list<std::string> l) : env(e), locals(std::move(l)) {}
  Env* env;
  std::list<std::string> locals;
};

/***** Frames and State *****/

struct Frame {
  std::string name;
  Cons<Scope*>* scopes;
  Cons<Action*>* todo;

  Frame(std::string n, Cons<Scope*>* s, Cons<Action*>* c)
      : name(std::move(std::move(n))), scopes(s), todo(c) {}
};

struct State {
  Cons<Frame*>* stack;
  std::vector<Value*> heap;
};

extern State* state;

void PrintEnv(Env* env);
auto AllocateValue(Value* v) -> Address;
auto CopyVal(Value* val, int line_num) -> Value*;
auto ToInteger(Value* v) -> int;

/***** Interpreters *****/

auto InterpProgram(std::list<Declaration*>* fs) -> int;
auto InterpExp(Env* env, Expression* e) -> Value*;

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_INTERPRETER_INTERPRETER_H