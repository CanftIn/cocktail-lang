#ifndef COCKTAIL_COMMON_META_PROGRAMMING_H
#define COCKTAIL_COMMON_META_PROGRAMMING_H

#include <type_traits>

namespace Cocktail {

template <typename... T, typename F>
constexpr auto Requires(F /*unused*/) -> bool {
  return std::is_invocable_v<F, T...>;
}

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_META_PROGRAMMING_H