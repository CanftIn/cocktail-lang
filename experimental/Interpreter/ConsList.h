#ifndef COCKTAIL_EXPERIMENTAL_INTERPRETER_CONS_LIST_H
#define COCKTAIL_EXPERIMENTAL_INTERPRETER_CONS_LIST_H

namespace Cocktail {

template <class T>
struct Cons {
  Cons(T e, Cons* n) : curr(e), next(n) {}

  T curr;
  Cons* next;
};

template <class T>
auto MakeCons(const T& x) -> Cons<T>* {
  return new Cons<T>(x, nullptr);
}

template <class T>
auto MakeCons(const T& x, Cons<T>* ls) -> Cons<T>* {
  return new Cons<T>(x, ls);
}

template <class T>
auto Length(Cons<T>* ls) -> unsigned int {
  if (ls) {
    return 1 + Length(ls->next);
  } else {
    return 0;
  }
}

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_INTERPRETER_CONS_LIST_H