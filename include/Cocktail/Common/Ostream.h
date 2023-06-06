#ifndef COCKTAIL_COMMON_OSTREAM_H
#define COCKTAIL_COMMON_OSTREAM_H

#include <type_traits>

#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

template <typename T,
          typename std::enable_if_t<std::is_member_function_pointer_v<
              decltype(&T::Print)>>* = nullptr>
auto operator<<(llvm::raw_ostream& os, const T& value) -> llvm::raw_ostream& {
  value.Print(os);
  return os;
}

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_OSTREAM_H