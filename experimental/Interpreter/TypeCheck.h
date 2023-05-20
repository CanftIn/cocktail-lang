#ifndef COCKTAIL_EXPERIMENTAL_INTERPRETER_TYPE_CHECK_H
#define COCKTAIL_EXPERIMENTAL_INTERPRETER_TYPE_CHECK_H

#include <set>

#include "experimental/AST/Expression.h"
#include "experimental/AST/Statement.h"
#include "experimental/Interpreter/AssocList.h"
#include "experimental/Interpreter/Interpreter.h"

namespace Cocktail {

using TypeEnv = AssocList<std::string, Value*>;

void PrintTypeEnv(TypeEnv* env);

enum class TCContext { ValueContext, PatternContext, TypeContext };

struct TCResult {
  TCResult(Expression* e, Value* t, TypeEnv* env) : exp(e), type(t), env(env) {}

  Expression* exp;
  Value* type;
  TypeEnv* env;
};

struct TCStatement {
  TCStatement(Statement* s, TypeEnv* e) : stmt(s), env(e) {}

  Statement* stmt;
  TypeEnv* env;
};

auto ToType(int line_num, Value* val) -> Value*;

auto TypeCheckExp(Expression* e, TypeEnv* env, Env* ct_env, Value* expected,
                  TCContext context) -> TCResult;

auto TypeCheckStmt(Statement*, TypeEnv*, Env*, Value*) -> TCStatement;

auto TypeCheckFunDef(struct FunctionDefinition*, TypeEnv*)
    -> struct FunctionDefinition*;

auto TypeCheckDecl(Declaration* d, TypeEnv* env, Env* ct_env) -> Declaration*;

auto TopLevel(std::list<Declaration*>* fs) -> std::pair<TypeEnv*, Env*>;

void PrintErrorString(const std::string& s);

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_INTERPRETER_TYPE_CHECK_H