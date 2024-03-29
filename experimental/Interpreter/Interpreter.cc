#include "experimental/Interpreter/Interpreter.h"

#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "experimental/AST/Expression.h"
#include "experimental/AST/FunctionDefinition.h"
#include "experimental/Interpreter/TypeCheck.h"

namespace Cocktail {

State* state = nullptr;

auto PatternMatch(Value* pat, Value* val, Env*, std::list<std::string>*, int)
    -> Env*;
void HandleValue();

template <class T>
static auto FindField(const std::string& field,
                      const std::vector<std::pair<std::string, T>>& inits)
    -> std::optional<T> {
  for (const auto& i : inits) {
    if (i.first == field) {
      return i.second;
    }
  }
  return std::nullopt;
}

/**** Auxiliary Functions ****/

auto AllocateValue(Value* v) -> Address {
  // Putting the following two side effects together in this function
  // ensures that we don't do anything else in between, which is really bad!
  // Consider whether to include a copy of the input v in this function
  // or to leave it up to the caller.
  Address a = state->heap.size();
  state->heap.push_back(v);
  return a;
}

auto CopyVal(Value* val, int line_num) -> Value* {
  CheckAlive(val, line_num);
  switch (val->tag) {
    case ValKind::TupleV: {
      auto elts = new std::vector<std::pair<std::string, Address>>();
      for (auto& i : *val->u.tuple.elts) {
        Value* elt = CopyVal(state->heap[i.second], line_num);
        elts->push_back(make_pair(i.first, AllocateValue(elt)));
      }
      return MakeTupleVal(elts);
    }
    case ValKind::AltV: {
      Value* arg = CopyVal(val->u.alt.arg, line_num);
      return MakeAltVal(*val->u.alt.alt_name, *val->u.alt.choice_name, arg);
    }
    case ValKind::StructV: {
      Value* inits = CopyVal(val->u.struct_val.inits, line_num);
      return MakeStructVal(val->u.struct_val.type, inits);
    }
    case ValKind::IntV:
      return MakeIntVal(val->u.integer);
    case ValKind::BoolV:
      return MakeBoolVal(val->u.boolean);
    case ValKind::FunV:
      return MakeFunVal(*val->u.fun.name, val->u.fun.param, val->u.fun.body);
    case ValKind::PtrV:
      return MakePtrVal(val->u.ptr);
    case ValKind::FunctionTV:
      return MakeFunTypeVal(CopyVal(val->u.fun_type.param, line_num),
                            CopyVal(val->u.fun_type.ret, line_num));

    case ValKind::PointerTV:
      return MakePtrTypeVal(CopyVal(val->u.ptr_type.type, line_num));
    case ValKind::IntTV:
      return MakeIntTypeVal();
    case ValKind::BoolTV:
      return MakeBoolTypeVal();
    case ValKind::TypeTV:
      return MakeTypeTypeVal();
    case ValKind::VarTV:
      return MakeVarTypeVal(*val->u.var_type);
    case ValKind::AutoTV:
      return MakeAutoTypeVal();
    case ValKind::TupleTV: {
      auto new_fields = new VarValues();
      for (auto& field : *val->u.tuple_type.fields) {
        auto v = CopyVal(field.second, line_num);
        new_fields->push_back(make_pair(field.first, v));
      }
      return MakeTupleTypeVal(new_fields);
    }
    case ValKind::StructTV:
    case ValKind::ChoiceTV:
    case ValKind::VarPatV:
    case ValKind::AltConsV:
      return val;  // no need to copy these because they are immutable?
      // No, they need to be copied so they don't get killed. -Jeremy
  }
}

void KillValue(Value* val) {
  val->alive = false;
  switch (val->tag) {
    case ValKind::AltV:
      KillValue(val->u.alt.arg);
      break;
    case ValKind::StructV:
      KillValue(val->u.struct_val.inits);
      break;
    case ValKind::TupleV:
      for (auto& elt : *val->u.tuple.elts) {
        if (state->heap[elt.second]->alive) {
          KillValue(state->heap[elt.second]);
        } else {
          std::cerr << "runtime error, killing an already dead value"
                    << std::endl;
          exit(-1);
        }
      }
      break;
    default:
      break;
  }
}

void PrintEnv(Env* env, std::ostream& out) {
  if (env) {
    std::cout << env->key << ": ";
    PrintValue(state->heap[env->value], out);
    std::cout << ", ";
    PrintEnv(env->next, out);
  }
}

/***** Frame and State Operations *****/

void PrintFrame(Frame* frame, std::ostream& out) {
  out << frame->name;
  out << "{";
  PrintActList(frame->todo, out);
  out << "}";
}

void PrintStack(Cons<Frame*>* ls, std::ostream& out) {
  if (ls) {
    PrintFrame(ls->curr, out);
    if (ls->next) {
      out << " :: ";
      PrintStack(ls->next, out);
    }
  }
}

void PrintHeap(const std::vector<Value*>& heap, std::ostream& out) {
  for (auto& iter : heap) {
    if (iter) {
      PrintValue(iter, out);
    } else {
      out << "_";
    }
    out << ", ";
  }
}

auto CurrentEnv(State* state) -> Env* {
  Frame* frame = state->stack->curr;
  return frame->scopes->curr->env;
}

void PrintState(std::ostream& out) {
  out << "{" << std::endl;
  out << "stack: ";
  PrintStack(state->stack, out);
  out << std::endl << "heap: ";
  PrintHeap(state->heap, out);
  out << std::endl << "env: ";
  PrintEnv(CurrentEnv(state), out);
  out << std::endl << "}" << std::endl;
}

/***** Auxiliary Functions *****/

auto ValToInt(Value* v, int line_num) -> int {
  CheckAlive(v, line_num);
  switch (v->tag) {
    case ValKind::IntV:
      return v->u.integer;
    default:
      std::cerr << line_num << ": runtime error: expected an integer"
                << std::endl;
      exit(-1);
  }
}

auto ValToBool(Value* v, int line_num) -> int {
  CheckAlive(v, line_num);
  switch (v->tag) {
    case ValKind::BoolV:
      return v->u.boolean;
    default:
      std::cerr << "runtime type error: expected a Boolean" << std::endl;
      exit(-1);
  }
}

auto ValToPtr(Value* v, int line_num) -> Address {
  CheckAlive(v, line_num);
  switch (v->tag) {
    case ValKind::PtrV:
      return v->u.ptr;
    default:
      std::cerr << "runtime type error: expected a pointer, not ";
      PrintValue(v, std::cerr);
      std::cerr << std::endl;
      exit(-1);
  }
}

auto EvalPrim(Operator op, const std::vector<Value*>& args, int line_num)
    -> Value* {
  switch (op) {
    case Operator::Neg:
      return MakeIntVal(-ValToInt(args[0], line_num));
    case Operator::Add:
      return MakeIntVal(ValToInt(args[0], line_num) +
                        ValToInt(args[1], line_num));
    case Operator::Sub:
      return MakeIntVal(ValToInt(args[0], line_num) -
                        ValToInt(args[1], line_num));
    case Operator::Not:
      return MakeBoolVal(!ValToBool(args[0], line_num));
    case Operator::And:
      return MakeBoolVal(ValToBool(args[0], line_num) &&
                         ValToBool(args[1], line_num));
    case Operator::Or:
      return MakeBoolVal(ValToBool(args[0], line_num) ||
                         ValToBool(args[1], line_num));
    case Operator::Eq:
      return MakeBoolVal(ValueEqual(args[0], args[1], line_num));
  }
}

Env* globals;

void InitGlobals(std::list<Declaration*>* fs) {
  globals = nullptr;
  for (auto& iter : *fs) {
    switch (iter->tag) {
      case DeclarationKind::ChoiceDeclaration: {
        auto d = iter;
        auto alts = new VarValues();
        for (auto i = d->u.choice_def.alternatives->begin();
             i != d->u.choice_def.alternatives->end(); ++i) {
          auto t =
              ToType(d->u.choice_def.line_num, InterpExp(nullptr, i->second));
          alts->push_back(make_pair(i->first, t));
        }
        auto ct = MakeChoiceTypeVal(d->u.choice_def.name, alts);
        auto a = AllocateValue(ct);
        globals = new Env(*d->u.choice_def.name, a, globals);
        break;
      }
      case DeclarationKind::StructDeclaration: {
        auto d = iter;
        auto fields = new VarValues();
        auto methods = new VarValues();
        for (auto i = d->u.struct_def->members->begin();
             i != d->u.struct_def->members->end(); ++i) {
          switch ((*i)->tag) {
            case MemberKind::FieldMember: {
              auto t = ToType(d->u.struct_def->line_num,
                              InterpExp(nullptr, (*i)->u.field.type));
              fields->push_back(make_pair(*(*i)->u.field.name, t));
              break;
            }
          }
        }
        auto st = MakeStructTypeVal(*d->u.struct_def->name, fields, methods);
        auto a = AllocateValue(st);
        globals = new Env(*d->u.struct_def->name, a, globals);
        break;
      }
      case DeclarationKind::FunctionDeclaration: {
        struct FunctionDefinition* fun = iter->u.fun_def;
        Env* env = nullptr;
        auto pt = InterpExp(env, fun->param_pattern);
        auto f = MakeFunVal(fun->name, pt, fun->body);
        Address a = AllocateValue(f);
        globals = new Env(fun->name, a, globals);
        break;
      }
    }
  }
}

//    { S, H} -> { { C, E, F} :: S, H}
// where C is the body of the function,
//       E is the environment (functions + parameters + locals)
//       F is the function
void CallFunction(int line_num, std::vector<Value*> operas, State* state) {
  CheckAlive(operas[0], line_num);
  switch (operas[0]->tag) {
    case ValKind::FunV: {
      // Bind arguments to parameters
      std::list<std::string> params;
      Env* env = PatternMatch(operas[0]->u.fun.param, operas[1], globals,
                              &params, line_num);
      if (!env) {
        std::cerr << "internal error in call_function, pattern match failed"
                  << std::endl;
        exit(-1);
      }
      // Create the new frame and push it on the stack
      auto* scope = new Scope(env, params);
      auto* frame = new Frame(*operas[0]->u.fun.name, MakeCons(scope),
                              MakeCons(MakeStmtAct(operas[0]->u.fun.body)));
      state->stack = MakeCons(frame, state->stack);
      break;
    }
    case ValKind::StructTV: {
      Value* arg = CopyVal(operas[1], line_num);
      Value* sv = MakeStructVal(operas[0], arg);
      Frame* frame = state->stack->curr;
      frame->todo = MakeCons(MakeValAct(sv), frame->todo);
      break;
    }
    case ValKind::AltConsV: {
      Value* arg = CopyVal(operas[1], line_num);
      Value* av = MakeAltVal(*operas[0]->u.alt_cons.alt_name,
                             *operas[0]->u.alt_cons.choice_name, arg);
      Frame* frame = state->stack->curr;
      frame->todo = MakeCons(MakeValAct(av), frame->todo);
      break;
    }
    default:
      std::cerr << line_num << ": in call, expected a function, not ";
      PrintValue(operas[0], std::cerr);
      std::cerr << std::endl;
      exit(-1);
  }
}

void KillScope(int line_num, Scope* scope) {
  for (const auto& l : scope->locals) {
    Address a = Lookup(line_num, scope->env, l, PrintErrorString);
    KillValue(state->heap[a]);
  }
}

void KillLocals(int line_num, Frame* frame) {
  Cons<Scope*>* scopes = frame->scopes;
  for (Scope* scope = scopes->curr; scopes; scopes = scopes->next) {
    KillScope(line_num, scope);
  }
}

void CreateTuple(Frame* frame, Action* act, Expression* /*exp*/) {
  //    { { (v1,...,vn) :: C, E, F} :: S, H}
  // -> { { `(v1,...,vn) :: C, E, F} :: S, H}
  auto elts = new std::vector<std::pair<std::string, Address>>();
  auto f = act->u.exp->u.tuple.fields->begin();
  for (auto i = act->results.begin(); i != act->results.end(); ++i, ++f) {
    Address a = AllocateValue(*i);  // copy?
    elts->push_back(make_pair(f->first, a));
  }
  Value* tv = MakeTupleVal(elts);
  frame->todo = MakeCons(MakeValAct(tv), frame->todo->next);
}

auto ToValue(Expression* value) -> Value* {
  switch (value->tag) {
    case ExpressionKind::Integer:
      return MakeIntVal(value->u.integer);
    case ExpressionKind::Boolean:
      return MakeBoolVal(value->u.boolean);
    case ExpressionKind::IntT:
      return MakeIntTypeVal();
    case ExpressionKind::BoolT:
      return MakeBoolTypeVal();
    case ExpressionKind::TypeT:
      return MakeTypeTypeVal();
    case ExpressionKind::FunctionT:
      // Instead add to patterns?
    default:
      std::cerr << "internal error in to_value, didn't expect ";
      PrintExp(value);
      std::cerr << std::endl;
      exit(-1);
  }
}

// Returns 0 if the value doesn't match the pattern.
auto PatternMatch(Value* p, Value* v, Env* env, std::list<std::string>* vars,
                  int line_num) -> Env* {
  std::cout << "pattern_match(";
  PrintValue(p, std::cout);
  std::cout << ", ";
  PrintValue(v, std::cout);
  std::cout << ")" << std::endl;
  switch (p->tag) {
    case ValKind::VarPatV: {
      Address a = AllocateValue(CopyVal(v, line_num));
      vars->push_back(*p->u.var_pat.name);
      return new Env(*p->u.var_pat.name, a, env);
    }
    case ValKind::TupleV:
      switch (v->tag) {
        case ValKind::TupleV: {
          if (p->u.tuple.elts->size() != v->u.tuple.elts->size()) {
            std::cerr << "runtime error: arity mismatch in tuple pattern match"
                      << std::endl;
            exit(-1);
          }
          for (auto& elt : *p->u.tuple.elts) {
            auto a = FindField(elt.first, *v->u.tuple.elts);
            if (a == std::nullopt) {
              std::cerr << "runtime error: field " << elt.first << "not in ";
              PrintValue(v, std::cerr);
              std::cerr << std::endl;
              exit(-1);
            }
            env = PatternMatch(state->heap[elt.second], state->heap[*a], env,
                               vars, line_num);
          }
          return env;
        }
        default:
          std::cerr
              << "internal error, expected a tuple value in pattern, not ";
          PrintValue(v, std::cerr);
          std::cerr << std::endl;
          exit(-1);
      }
    case ValKind::AltV:
      switch (v->tag) {
        case ValKind::AltV: {
          if (*p->u.alt.choice_name != *v->u.alt.choice_name ||
              *p->u.alt.alt_name != *v->u.alt.alt_name) {
            return nullptr;
          }
          env = PatternMatch(p->u.alt.arg, v->u.alt.arg, env, vars, line_num);
          return env;
        }
        default:
          std::cerr
              << "internal error, expected a choice alternative in pattern, "
                 "not ";
          PrintValue(v, std::cerr);
          std::cerr << std::endl;
          exit(-1);
      }
    case ValKind::FunctionTV:
      switch (v->tag) {
        case ValKind::FunctionTV:
          env = PatternMatch(p->u.fun_type.param, v->u.fun_type.param, env,
                             vars, line_num);
          env = PatternMatch(p->u.fun_type.ret, v->u.fun_type.ret, env, vars,
                             line_num);
          return env;
        default:
          return nullptr;
      }
    default:
      if (ValueEqual(p, v, line_num)) {
        return env;
      } else {
        return nullptr;
      }
  }
}

void PatternAssignment(Value* pat, Value* val, int line_num) {
  switch (pat->tag) {
    case ValKind::PtrV:
      state->heap[ValToPtr(pat, line_num)] = val;
      break;
    case ValKind::TupleV: {
      switch (val->tag) {
        case ValKind::TupleV: {
          if (pat->u.tuple.elts->size() != val->u.tuple.elts->size()) {
            std::cerr << "runtime error: arity mismatch in tuple pattern match"
                      << std::endl;
            exit(-1);
          }
          for (auto& elt : *pat->u.tuple.elts) {
            auto a = FindField(elt.first, *val->u.tuple.elts);
            if (a == std::nullopt) {
              std::cerr << "runtime error: field " << elt.first << "not in ";
              PrintValue(val, std::cerr);
              std::cerr << std::endl;
              exit(-1);
            }
            PatternAssignment(state->heap[elt.second], state->heap[*a],
                              line_num);
          }
          break;
        }
        default:
          std::cerr
              << "internal error, expected a tuple value on right-hand-side, "
                 "not ";
          PrintValue(val, std::cerr);
          std::cerr << std::endl;
          exit(-1);
      }
      break;
    }
    case ValKind::AltV: {
      switch (val->tag) {
        case ValKind::AltV: {
          if (*pat->u.alt.choice_name != *val->u.alt.choice_name ||
              *pat->u.alt.alt_name != *val->u.alt.alt_name) {
            std::cerr << "internal error in pattern assignment" << std::endl;
            exit(-1);
          }
          PatternAssignment(pat->u.alt.arg, val->u.alt.arg, line_num);
          break;
        }
        default:
          std::cerr
              << "internal error, expected an alternative in left-hand-side, "
                 "not ";
          PrintValue(val, std::cerr);
          std::cerr << std::endl;
          exit(-1);
      }
      break;
    }
    default:
      if (!ValueEqual(pat, val, line_num)) {
        std::cerr << "internal error in pattern assignment" << std::endl;
        exit(-1);
      }
  }
}

/***** state transitions for lvalues *****/

void StepLvalue() {
  Frame* frame = state->stack->curr;
  Action* act = frame->todo->curr;
  Expression* exp = act->u.exp;
  std::cout << "--- step lvalue ";
  PrintExp(exp);
  std::cout << " --->" << std::endl;
  switch (exp->tag) {
    case ExpressionKind::Variable: {
      //    { {x :: C, E, F} :: S, H}
      // -> { {E(x) :: C, E, F} :: S, H}
      Address a = Lookup(exp->line_num, CurrentEnv(state),
                         *(exp->u.variable.name), PrintErrorString);
      Value* v = MakePtrVal(a);
      CheckAlive(v, exp->line_num);
      frame->todo = MakeCons(MakeValAct(v), frame->todo->next);
      break;
    }
    case ExpressionKind::GetField: {
      //    { {e.f :: C, E, F} :: S, H}
      // -> { e :: [].f :: C, E, F} :: S, H}
      frame->todo =
          MakeCons(MakeLvalAct(exp->u.get_field.aggregate), frame->todo);
      act->pos++;
      break;
    }
    case ExpressionKind::Index: {
      //    { {e[i] :: C, E, F} :: S, H}
      // -> { e :: [][i] :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeExpAct(exp->u.index.aggregate), frame->todo);
      act->pos++;
      break;
    }
    case ExpressionKind::Tuple: {
      //    { {(f1=e1,...) :: C, E, F} :: S, H}
      // -> { {e1 :: (f1=[],...) :: C, E, F} :: S, H}
      Expression* e1 = (*exp->u.tuple.fields)[0].second;
      frame->todo = MakeCons(MakeLvalAct(e1), frame->todo);
      act->pos++;
      break;
    }
    case ExpressionKind::Integer:
    case ExpressionKind::Boolean:
    case ExpressionKind::Call:
    case ExpressionKind::PrimitiveOp:
    case ExpressionKind::IntT:
    case ExpressionKind::BoolT:
    case ExpressionKind::TypeT:
    case ExpressionKind::FunctionT:
    case ExpressionKind::AutoT:
    case ExpressionKind::PatternVariable: {
      frame->todo = MakeCons(MakeExpAct(exp),
                             MakeCons(MakeExpToLvalAct(), frame->todo->next));
    }
  }
}

/***** state transitions for expressions *****/

void StepExp() {
  Frame* frame = state->stack->curr;
  Action* act = frame->todo->curr;
  Expression* exp = act->u.exp;
  std::cout << "--- step exp ";
  PrintExp(exp);
  std::cout << " --->" << std::endl;
  switch (exp->tag) {
    case ExpressionKind::PatternVariable: {
      frame->todo =
          MakeCons(MakeExpAct(exp->u.pattern_variable.type), frame->todo);
      act->pos++;
      break;
    }
    case ExpressionKind::Index: {
      //    { { e[i] :: C, E, F} :: S, H}
      // -> { { e :: [][i] :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeExpAct(exp->u.index.aggregate), frame->todo);
      act->pos++;
      break;
    }
    case ExpressionKind::Tuple: {
      if (exp->u.tuple.fields->size() > 0) {
        //    { {(f1=e1,...) :: C, E, F} :: S, H}
        // -> { {e1 :: (f1=[],...) :: C, E, F} :: S, H}
        Expression* e1 = (*exp->u.tuple.fields)[0].second;
        frame->todo = MakeCons(MakeExpAct(e1), frame->todo);
        act->pos++;
      } else {
        CreateTuple(frame, act, exp);
      }
      break;
    }
    case ExpressionKind::GetField: {
      //    { { e.f :: C, E, F} :: S, H}
      // -> { { e :: [].f :: C, E, F} :: S, H}
      frame->todo =
          MakeCons(MakeLvalAct(exp->u.get_field.aggregate), frame->todo);
      act->pos++;
      break;
    }
    case ExpressionKind::Variable: {
      // { {x :: C, E, F} :: S, H} -> { {H(E(x)) :: C, E, F} :: S, H}
      Address a = Lookup(exp->line_num, CurrentEnv(state),
                         *(exp->u.variable.name), PrintErrorString);
      Value* v = state->heap[a];
      frame->todo = MakeCons(MakeValAct(v), frame->todo->next);
      break;
    }
    case ExpressionKind::Integer:
      // { {n :: C, E, F} :: S, H} -> { {n' :: C, E, F} :: S, H}
      frame->todo =
          MakeCons(MakeValAct(MakeIntVal(exp->u.integer)), frame->todo->next);
      break;
    case ExpressionKind::Boolean:
      // { {n :: C, E, F} :: S, H} -> { {n' :: C, E, F} :: S, H}
      frame->todo =
          MakeCons(MakeValAct(MakeBoolVal(exp->u.boolean)), frame->todo->next);
      break;
    case ExpressionKind::PrimitiveOp:
      if (exp->u.primitive_op.arguments->size() > 0) {
        //    { {op(e :: es) :: C, E, F} :: S, H}
        // -> { e :: op([] :: es) :: C, E, F} :: S, H}
        frame->todo = MakeCons(
            MakeExpAct(exp->u.primitive_op.arguments->front()), frame->todo);
        act->pos++;
      } else {
        //    { {v :: op(]) :: C, E, F} :: S, H}
        // -> { {eval_prim(op, ()) :: C, E, F} :: S, H}
        Value* v =
            EvalPrim(exp->u.primitive_op.op, act->results, exp->line_num);
        frame->todo = MakeCons(MakeValAct(v), frame->todo->next->next);
      }
      break;
    case ExpressionKind::Call:
      //    { {e1(e2) :: C, E, F} :: S, H}
      // -> { {e1 :: [](e2) :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeExpAct(exp->u.call.function), frame->todo);
      act->pos++;
      break;
    case ExpressionKind::IntT: {
      Value* v = MakeIntTypeVal();
      frame->todo = MakeCons(MakeValAct(v), frame->todo->next);
      break;
    }
    case ExpressionKind::BoolT: {
      Value* v = MakeBoolTypeVal();
      frame->todo = MakeCons(MakeValAct(v), frame->todo->next);
      break;
    }
    case ExpressionKind::AutoT: {
      Value* v = MakeAutoTypeVal();
      frame->todo = MakeCons(MakeValAct(v), frame->todo->next);
      break;
    }
    case ExpressionKind::TypeT: {
      Value* v = MakeTypeTypeVal();
      frame->todo = MakeCons(MakeValAct(v), frame->todo->next);
      break;
    }
    case ExpressionKind::FunctionT: {
      frame->todo =
          MakeCons(MakeExpAct(exp->u.function_type.parameter), frame->todo);
      act->pos++;
      break;
    }
  }  // switch (exp->tag)
}

/***** state transitions for statements *****/

auto IsWhileAct(Action* act) -> bool {
  switch (act->tag) {
    case ActionKind::StatementAction:
      switch (act->u.stmt->tag) {
        case StatementKind::While:
          return true;
        default:
          return false;
      }
    default:
      return false;
  }
}

auto IsBlockAct(Action* act) -> bool {
  switch (act->tag) {
    case ActionKind::StatementAction:
      switch (act->u.stmt->tag) {
        case StatementKind::Block:
          return true;
        default:
          return false;
      }
    default:
      return false;
  }
}

void StepStmt() {
  Frame* frame = state->stack->curr;
  Action* act = frame->todo->curr;
  Statement* stmt = act->u.stmt;
  std::cout << "--- step stmt ";
  PrintStatement(stmt, 1);
  std::cout << " --->" << std::endl;
  switch (stmt->tag) {
    case StatementKind::Match:
      //    { { (match (e) ...) :: C, E, F} :: S, H}
      // -> { { e :: (match ([]) ...) :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeExpAct(stmt->u.match_stmt.exp), frame->todo);
      act->pos++;
      break;
    case StatementKind::While:
      //    { { (while (e) s) :: C, E, F} :: S, H}
      // -> { { e :: (while ([]) s) :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeExpAct(stmt->u.while_stmt.cond), frame->todo);
      act->pos++;
      break;
    case StatementKind::Break:
      //    { { break; :: ... :: (while (e) s) :: C, E, F} :: S, H}
      // -> { { C, E', F} :: S, H}
      frame->todo = frame->todo->next;
      while (frame->todo && !IsWhileAct(frame->todo->curr)) {
        if (IsBlockAct(frame->todo->curr)) {
          KillScope(stmt->line_num, frame->scopes->curr);
          frame->scopes = frame->scopes->next;
        }
        frame->todo = frame->todo->next;
      }
      frame->todo = frame->todo->next;
      break;
    case StatementKind::Continue:
      //    { { continue; :: ... :: (while (e) s) :: C, E, F} :: S, H}
      // -> { { (while (e) s) :: C, E', F} :: S, H}
      frame->todo = frame->todo->next;
      while (frame->todo && !IsWhileAct(frame->todo->curr)) {
        if (IsBlockAct(frame->todo->curr)) {
          KillScope(stmt->line_num, frame->scopes->curr);
          frame->scopes = frame->scopes->next;
        }
        frame->todo = frame->todo->next;
      }
      break;
    case StatementKind::Block: {
      if (act->pos == -1) {
        auto* scope = new Scope(CurrentEnv(state), std::list<std::string>());
        frame->scopes = MakeCons(scope, frame->scopes);
        frame->todo = MakeCons(MakeStmtAct(stmt->u.block.stmt), frame->todo);
        act->pos++;
      } else {
        Scope* scope = frame->scopes->curr;
        KillScope(stmt->line_num, scope);
        frame->scopes = frame->scopes->next;
        frame->todo = frame->todo->next;
      }
      break;
    }
    case StatementKind::VariableDefinition:
      //    { {(var x = e) :: C, E, F} :: S, H}
      // -> { {e :: (var x = []) :: C, E, F} :: S, H}
      frame->todo =
          MakeCons(MakeExpAct(stmt->u.variable_definition.init), frame->todo);
      act->pos++;
      break;
    case StatementKind::ExpressionStatement:
      //    { {e :: C, E, F} :: S, H}
      // -> { {e :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeExpAct(stmt->u.exp), frame->todo);
      break;
    case StatementKind::Assign:
      //    { {(lv = e) :: C, E, F} :: S, H}
      // -> { {lv :: ([] = e) :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeLvalAct(stmt->u.assign.lhs), frame->todo);
      act->pos++;
      break;
    case StatementKind::If:
      //    { {(if (e) then_stmt else else_stmt) :: C, E, F} :: S, H}
      // -> { { e :: (if ([]) then_stmt else else_stmt) :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeExpAct(stmt->u.if_stmt.cond), frame->todo);
      act->pos++;
      break;
    case StatementKind::Return:
      //    { {return e :: C, E, F} :: S, H}
      // -> { {e :: return [] :: C, E, F} :: S, H}
      frame->todo = MakeCons(MakeExpAct(stmt->u.return_stmt), frame->todo);
      act->pos++;
      break;
    case StatementKind::Sequence:
      //    { { (s1,s2) :: C, E, F} :: S, H}
      // -> { { s1 :: s2 :: C, E, F} :: S, H}
      Cons<Action*>* todo = frame->todo->next;
      if (stmt->u.sequence.next) {
        todo = MakeCons(MakeStmtAct(stmt->u.sequence.next), todo);
      }
      frame->todo = MakeCons(MakeStmtAct(stmt->u.sequence.stmt), todo);
      break;
  }
}

auto GetMember(Address a, const std::string& f) -> Address {
  Value* v = state->heap[a];
  switch (v->tag) {
    case ValKind::StructV: {
      auto a = FindField(f, *v->u.struct_val.inits->u.tuple.elts);
      if (a == std::nullopt) {
        std::cerr << "runtime error, member " << f << " not in ";
        PrintValue(v, std::cerr);
        std::cerr << std::endl;
        exit(-1);
      }
      return *a;
    }
    case ValKind::TupleV: {
      auto a = FindField(f, *v->u.tuple.elts);
      if (a == std::nullopt) {
        std::cerr << "field " << f << " not in ";
        PrintValue(v, std::cerr);
        std::cerr << std::endl;
        exit(-1);
      }
      return *a;
    }
    case ValKind::ChoiceTV: {
      if (FindInVarValues(f, v->u.choice_type.alternatives) == nullptr) {
        std::cerr << "alternative " << f << " not in ";
        PrintValue(v, std::cerr);
        std::cerr << std::endl;
        exit(-1);
      }
      auto ac = MakeAltCons(f, *v->u.choice_type.name);
      return AllocateValue(ac);
    }
    default:
      std::cerr << "field access not allowed for value ";
      PrintValue(v, std::cerr);
      std::cerr << std::endl;
      exit(-1);
  }
}

auto InsertDelete(Action* del, Cons<Action*>* todo) -> Cons<Action*>* {
  if (todo) {
    switch (todo->curr->tag) {
      case ActionKind::StatementAction: {
        // This places the delete before the enclosing statement.
        // Not sure if that is OK. Conceptually it should go after
        // but that is tricky for some statements, like 'return'. -Jeremy
        return MakeCons(del, todo);
      }
      case ActionKind::LValAction:
      case ActionKind::ExpressionAction:
      case ActionKind::ValAction:
      case ActionKind::ExpToLValAction:
      case ActionKind::DeleteTmpAction:
        return MakeCons(todo->curr, InsertDelete(del, todo->next));
    }
  } else {
    return MakeCons(del, todo);
  }
}

/***** State transition for handling a value *****/

void HandleValue() {
  Frame* frame = state->stack->curr;
  Action* val_act = frame->todo->curr;
  Action* act = frame->todo->next->curr;
  act->results.push_back(val_act->u.val);
  act->pos++;

  std::cout << "--- handle value ";
  PrintValue(val_act->u.val, std::cout);
  std::cout << " with ";
  PrintAct(act, std::cout);
  std::cout << " --->" << std::endl;

  switch (act->tag) {
    case ActionKind::DeleteTmpAction: {
      KillValue(state->heap[act->u.delete_tmp]);
      frame->todo = MakeCons(val_act, frame->todo->next->next);
      break;
    }
    case ActionKind::ExpToLValAction: {
      Address a = AllocateValue(act->results[0]);
      auto del = MakeDeleteAct(a);
      frame->todo = MakeCons(MakeValAct(MakePtrVal(a)),
                             InsertDelete(del, frame->todo->next->next));
      break;
    }
    case ActionKind::LValAction: {
      Expression* exp = act->u.exp;
      switch (exp->tag) {
        case ExpressionKind::GetField: {
          //    { v :: [].f :: C, E, F} :: S, H}
          // -> { { &v.f :: C, E, F} :: S, H }
          Value* str = act->results[0];
          Address a =
              GetMember(ValToPtr(str, exp->line_num), *exp->u.get_field.field);
          frame->todo =
              MakeCons(MakeValAct(MakePtrVal(a)), frame->todo->next->next);
          break;
        }
        case ExpressionKind::Index: {
          if (act->pos == 1) {
            frame->todo =
                MakeCons(MakeExpAct(exp->u.index.offset), frame->todo->next);
          } else if (act->pos == 2) {
            //    { v :: [][i] :: C, E, F} :: S, H}
            // -> { { &v[i] :: C, E, F} :: S, H }
            Value* tuple = act->results[0];
            std::string f = std::to_string(ToInteger(act->results[1]));
            auto a = FindField(f, *tuple->u.tuple.elts);
            if (a == std::nullopt) {
              std::cerr << "runtime error: field " << f << "not in ";
              PrintValue(tuple, std::cerr);
              std::cerr << std::endl;
              exit(-1);
            }
            frame->todo =
                MakeCons(MakeValAct(MakePtrVal(*a)), frame->todo->next->next);
          }
          break;
        }
        case ExpressionKind::Tuple: {
          if (act->pos != static_cast<int>(exp->u.tuple.fields->size())) {
            //    { { vk :: (f1=v1,..., fk=[],fk+1=ek+1,...) :: C, E, F} :: S,
            //    H}
            // -> { { ek+1 :: (f1=v1,..., fk=vk, fk+1=[],...) :: C, E, F} :: S,
            // H}
            Expression* elt = (*exp->u.tuple.fields)[act->pos].second;
            frame->todo = MakeCons(MakeLvalAct(elt), frame->todo->next);
          } else {
            frame->todo = frame->todo->next;
            CreateTuple(frame, act, exp);
          }
          break;
        }
        default:
          std::cerr << "internal error in handle_value, LValAction"
                    << std::endl;
          exit(-1);
      }
      break;
    }
    case ActionKind::ExpressionAction: {
      Expression* exp = act->u.exp;
      switch (exp->tag) {
        case ExpressionKind::PatternVariable: {
          auto v =
              MakeVarPatVal(*exp->u.pattern_variable.name, act->results[0]);
          frame->todo = MakeCons(MakeValAct(v), frame->todo->next->next);
          break;
        }
        case ExpressionKind::Tuple: {
          if (act->pos != static_cast<int>(exp->u.tuple.fields->size())) {
            //    { { vk :: (f1=v1,..., fk=[],fk+1=ek+1,...) :: C, E, F} :: S,
            //    H}
            // -> { { ek+1 :: (f1=v1,..., fk=vk, fk+1=[],...) :: C, E, F} :: S,
            // H}
            Expression* elt = (*exp->u.tuple.fields)[act->pos].second;
            frame->todo = MakeCons(MakeExpAct(elt), frame->todo->next);
          } else {
            frame->todo = frame->todo->next;
            CreateTuple(frame, act, exp);
          }
          break;
        }
        case ExpressionKind::Index: {
          if (act->pos == 1) {
            frame->todo =
                MakeCons(MakeExpAct(exp->u.index.offset), frame->todo->next);
          } else if (act->pos == 2) {
            auto tuple = act->results[0];
            switch (tuple->tag) {
              case ValKind::TupleV: {
                //    { { v :: [][i] :: C, E, F} :: S, H}
                // -> { { v_i :: C, E, F} : S, H}
                std::string f = std::to_string(ToInteger(act->results[1]));
                auto a = FindField(f, *tuple->u.tuple.elts);
                if (a == std::nullopt) {
                  std::cerr << "runtime error, field " << f << " not in ";
                  PrintValue(tuple, std::cerr);
                  std::cerr << std::endl;
                  exit(-1);
                }
                frame->todo = MakeCons(MakeValAct(state->heap[*a]),
                                       frame->todo->next->next);
                break;
              }
              default:
                std::cerr
                    << "runtime type error, expected a tuple in field access, "
                       "not ";
                PrintValue(tuple, std::cerr);
                exit(-1);
            }
          }
          break;
        }
        case ExpressionKind::GetField: {
          //    { { v :: [].f :: C, E, F} :: S, H}
          // -> { { v_f :: C, E, F} : S, H}
          auto a = GetMember(ValToPtr(act->results[0], exp->line_num),
                             *exp->u.get_field.field);
          frame->todo =
              MakeCons(MakeValAct(state->heap[a]), frame->todo->next->next);
          break;
        }
        case ExpressionKind::PrimitiveOp: {
          if (act->pos !=
              static_cast<int>(exp->u.primitive_op.arguments->size())) {
            //    { {v :: op(vs,[],e,es) :: C, E, F} :: S, H}
            // -> { {e :: op(vs,v,[],es) :: C, E, F} :: S, H}
            Expression* arg = (*exp->u.primitive_op.arguments)[act->pos];
            frame->todo = MakeCons(MakeExpAct(arg), frame->todo->next);
          } else {
            //    { {v :: op(vs,[]) :: C, E, F} :: S, H}
            // -> { {eval_prim(op, (vs,v)) :: C, E, F} :: S, H}
            Value* v =
                EvalPrim(exp->u.primitive_op.op, act->results, exp->line_num);
            frame->todo = MakeCons(MakeValAct(v), frame->todo->next->next);
          }
          break;
        }
        case ExpressionKind::Call: {
          if (act->pos == 1) {
            //    { { v :: [](e) :: C, E, F} :: S, H}
            // -> { { e :: v([]) :: C, E, F} :: S, H}
            frame->todo =
                MakeCons(MakeExpAct(exp->u.call.argument), frame->todo->next);
          } else if (act->pos == 2) {
            //    { { v2 :: v1([]) :: C, E, F} :: S, H}
            // -> { {C',E',F'} :: {C, E, F} :: S, H}
            frame->todo = frame->todo->next->next;
            CallFunction(exp->line_num, act->results, state);
          } else {
            std::cerr << "internal error in handle_value with Call"
                      << std::endl;
            exit(-1);
          }
          break;
        }
        case ExpressionKind::FunctionT: {
          if (act->pos == 2) {
            //    { { rt :: fn pt -> [] :: C, E, F} :: S, H}
            // -> { fn pt -> rt :: {C, E, F} :: S, H}
            Value* v = MakeFunTypeVal(act->results[0], act->results[1]);
            frame->todo = MakeCons(MakeValAct(v), frame->todo->next->next);
          } else {
            //    { { pt :: fn [] -> e :: C, E, F} :: S, H}
            // -> { { e :: fn pt -> []) :: C, E, F} :: S, H}
            frame->todo = MakeCons(MakeExpAct(exp->u.function_type.return_type),
                                   frame->todo->next);
          }
          break;
        }
        case ExpressionKind::Variable:
        case ExpressionKind::Integer:
        case ExpressionKind::Boolean:
        case ExpressionKind::IntT:
        case ExpressionKind::BoolT:
        case ExpressionKind::TypeT:
        case ExpressionKind::AutoT:
          std::cerr << "internal error, bad expression context in handle_value"
                    << std::endl;
          exit(-1);
      }
      break;
    }
    case ActionKind::StatementAction: {
      Statement* stmt = act->u.stmt;
      switch (stmt->tag) {
        case StatementKind::ExpressionStatement:
          frame->todo = frame->todo->next->next;
          break;
        case StatementKind::VariableDefinition: {
          if (act->pos == 1) {
            frame->todo = MakeCons(MakeExpAct(stmt->u.variable_definition.pat),
                                   frame->todo->next);
          } else if (act->pos == 2) {
            //    { { v :: (x = []) :: C, E, F} :: S, H}
            // -> { { C, E(x := a), F} :: S, H(a := copy(v))}
            Value* v = act->results[0];
            Value* p = act->results[1];
            // Address a = AllocateValue(CopyVal(v));
            frame->scopes->curr->env =
                PatternMatch(p, v, frame->scopes->curr->env,
                             &frame->scopes->curr->locals, stmt->line_num);
            if (!frame->scopes->curr->env) {
              std::cerr
                  << stmt->line_num
                  << ": internal error in variable definition, match failed"
                  << std::endl;
              exit(-1);
            }
            frame->todo = frame->todo->next->next;
          }
          break;
        }
        case StatementKind::Assign:
          if (act->pos == 1) {
            //    { { a :: ([] = e) :: C, E, F} :: S, H}
            // -> { { e :: (a = []) :: C, E, F} :: S, H}
            frame->todo =
                MakeCons(MakeExpAct(stmt->u.assign.rhs), frame->todo->next);
          } else if (act->pos == 2) {
            //    { { v :: (a = []) :: C, E, F} :: S, H}
            // -> { { C, E, F} :: S, H(a := v)}
            auto pat = act->results[0];
            auto val = act->results[1];
            PatternAssignment(pat, val, stmt->line_num);
            frame->todo = frame->todo->next->next;
          }
          break;
        case StatementKind::If:
          if (ValToBool(act->results[0], stmt->line_num)) {
            //    { {true :: if ([]) then_stmt else else_stmt :: C, E, F} ::
            //      S, H}
            // -> { { then_stmt :: C, E, F } :: S, H}
            frame->todo = MakeCons(MakeStmtAct(stmt->u.if_stmt.then_stmt),
                                   frame->todo->next->next);
          } else {
            //    { {false :: if ([]) then_stmt else else_stmt :: C, E, F} ::
            //      S, H}
            // -> { { else_stmt :: C, E, F } :: S, H}
            frame->todo = MakeCons(MakeStmtAct(stmt->u.if_stmt.else_stmt),
                                   frame->todo->next->next);
          }
          break;
        case StatementKind::While:
          if (ValToBool(act->results[0], stmt->line_num)) {
            //    { {true :: (while ([]) s) :: C, E, F} :: S, H}
            // -> { { s :: (while (e) s) :: C, E, F } :: S, H}
            frame->todo->next->curr->pos = -1;
            frame->todo->next->curr->results.clear();
            frame->todo = MakeCons(MakeStmtAct(stmt->u.while_stmt.body),
                                   frame->todo->next);
          } else {
            //    { {false :: (while ([]) s) :: C, E, F} :: S, H}
            // -> { { C, E, F } :: S, H}
            frame->todo->next->curr->pos = -1;
            frame->todo->next->curr->results.clear();
            frame->todo = frame->todo->next->next;
          }
          break;
        case StatementKind::Match: {
          // Regarding act->pos:
          // * odd: start interpreting the pattern of a clause
          // * even: finished interpreting the pattern, now try to match
          //
          // Regarding act->results:
          // * 0: the value that we're matching
          // * 1: the pattern for clause 0
          // * 2: the pattern for clause 1
          // * ...
          auto clause_num = (act->pos - 1) / 2;
          if (clause_num >=
              static_cast<int>(stmt->u.match_stmt.clauses->size())) {
            frame->todo = frame->todo->next->next;
            break;
          }
          auto c = stmt->u.match_stmt.clauses->begin();
          std::advance(c, clause_num);

          if (act->pos % 2 == 1) {
            // start interpreting the pattern of the clause
            //    { {v :: (match ([]) ...) :: C, E, F} :: S, H}
            // -> { {pi :: (match ([]) ...) :: C, E, F} :: S, H}
            frame->todo = MakeCons(MakeExpAct(c->first), frame->todo->next);
          } else {  // try to match
            auto v = act->results[0];
            auto pat = act->results[clause_num + 1];
            auto env = CurrentEnv(state);
            std::list<std::string> vars;
            Env* new_env = PatternMatch(pat, v, env, &vars, stmt->line_num);
            if (new_env) {  // we have a match, start the body
              auto* new_scope = new Scope(new_env, vars);
              frame->scopes = MakeCons(new_scope, frame->scopes);
              Statement* body_block = MakeBlock(stmt->line_num, c->second);
              Action* body_act = MakeStmtAct(body_block);
              body_act->pos = 0;
              frame->todo =
                  MakeCons(MakeStmtAct(c->second),
                           MakeCons(body_act, frame->todo->next->next));
            } else {
              act->pos++;
              clause_num = (act->pos - 1) / 2;
              if (clause_num <
                  static_cast<int>(stmt->u.match_stmt.clauses->size())) {
                // move on to the next clause
                c = stmt->u.match_stmt.clauses->begin();
                std::advance(c, clause_num);
                frame->todo = MakeCons(MakeExpAct(c->first), frame->todo->next);
              } else {  // No more clauses in match
                frame->todo = frame->todo->next->next;
              }
            }
          }
          break;
        }
        case StatementKind::Return: {
          //    { {v :: return [] :: C, E, F} :: {C', E', F'} :: S, H}
          // -> { {v :: C', E', F'} :: S, H}
          Value* ret_val = CopyVal(val_act->u.val, stmt->line_num);
          KillLocals(stmt->line_num, frame);
          state->stack = state->stack->next;
          frame = state->stack->curr;
          frame->todo = MakeCons(MakeValAct(ret_val), frame->todo);
          break;
        }
        case StatementKind::Block:
        case StatementKind::Sequence:
        case StatementKind::Break:
        case StatementKind::Continue:
          std::cerr << "internal error in handle_value, unhandled statement ";
          PrintStatement(stmt, 1);
          std::cerr << std::endl;
          exit(-1);
      }  // switch stmt
      break;
    }
    case ActionKind::ValAction:
      std::cerr << "internal error, ValAction in handle_value" << std::endl;
      exit(-1);
  }  // switch act
}

// State transition.
void Step() {
  Frame* frame = state->stack->curr;
  if (!frame->todo) {
    std::cerr << "runtime error: fell off end of function " << frame->name
              << " without `return`" << std::endl;
    exit(-1);
  }

  Action* act = frame->todo->curr;
  switch (act->tag) {
    case ActionKind::DeleteTmpAction:
      std::cerr << "internal error in step, did not expect DeleteTmpAction"
                << std::endl;
      break;
    case ActionKind::ExpToLValAction:
      std::cerr << "internal error in step, did not expect ExpToLValAction"
                << std::endl;
      break;
    case ActionKind::ValAction:
      HandleValue();
      break;
    case ActionKind::LValAction:
      StepLvalue();
      break;
    case ActionKind::ExpressionAction:
      StepExp();
      break;
    case ActionKind::StatementAction:
      StepStmt();
      break;
  }  // switch
}

// Interpret the whole porogram.
auto InterpProgram(std::list<Declaration*>* fs) -> int {
  state = new State();  // Runtime state.
  std::cout << "********** initializing globals **********" << std::endl;
  InitGlobals(fs);

  Expression* arg =
      MakeTuple(0, new std::vector<std::pair<std::string, Expression*>>());
  Expression* call_main = MakeCall(0, MakeVar(0, "main"), arg);
  Cons<Action*>* todo = MakeCons(MakeExpAct(call_main));
  auto* scope = new Scope(globals, std::list<std::string>());
  auto* frame = new Frame("top", MakeCons(scope), todo);
  state->stack = MakeCons(frame);

  std::cout << "********** calling main function **********" << std::endl;
  PrintState(std::cout);

  while (Length(state->stack) > 1 || Length(state->stack->curr->todo) > 1 ||
         state->stack->curr->todo->curr->tag != ActionKind::ValAction) {
    Step();
    PrintState(std::cout);
  }
  Value* v = state->stack->curr->todo->curr->u.val;
  return ValToInt(v, 0);
}

// Interpret an expression at compile-time.
auto InterpExp(Env* env, Expression* e) -> Value* {
  Cons<Action*>* todo = MakeCons(MakeExpAct(e));
  auto* scope = new Scope(env, std::list<std::string>());
  auto* frame = new Frame("InterpExp", MakeCons(scope), todo);
  state->stack = MakeCons(frame);

  while (Length(state->stack) > 1 || Length(state->stack->curr->todo) > 1 ||
         state->stack->curr->todo->curr->tag != ActionKind::ValAction) {
    Step();
  }
  Value* v = state->stack->curr->todo->curr->u.val;
  return v;
}

}  // namespace Cocktail