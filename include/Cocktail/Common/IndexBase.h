#ifndef COCKTAIL_COMMON_INDEX_BASE_H
#define COCKTAIL_COMMON_INDEX_BASE_H

#include <type_traits>

#include "Cocktail/Common/Ostream.h"

namespace Cocktail {

template <typename DataType>
class DataIterator;

// vector中元素的轻量级句柄。
// DataIndex被设计为通过值传递，而不是引用或指针。
struct IndexBase : public Printable<IndexBase> {
  static constexpr int32_t InvalidIndex = -1;

  IndexBase() = delete;
  constexpr explicit IndexBase(int index) : index(index) {}

  auto Print(llvm::raw_ostream& output) const -> void {
    if (is_valid()) {
      output << index;
    } else {
      output << "<invalid>";
    }
  }

  auto is_valid() const -> bool { return index != InvalidIndex; }

  int32_t index;
};

// 提供 < 和 > 比较操作。
struct ComparableIndexBase : public IndexBase {
  using IndexBase::IndexBase;
};

template <typename IndexType,
          typename std::enable_if_t<std::is_base_of_v<IndexBase, IndexType>>* =
              nullptr>
auto operator==(IndexType lhs, IndexType rhs) -> bool {
  return lhs.index == rhs.index;
}
template <typename IndexType,
          typename std::enable_if_t<std::is_base_of_v<IndexBase, IndexType>>* =
              nullptr>
auto operator!=(IndexType lhs, IndexType rhs) -> bool {
  return lhs.index != rhs.index;
}

template <typename IndexType, typename std::enable_if_t<std::is_base_of_v<
                                  ComparableIndexBase, IndexType>>* = nullptr>
auto operator<(IndexType lhs, IndexType rhs) -> bool {
  return lhs.index < rhs.index;
}
template <typename IndexType, typename std::enable_if_t<std::is_base_of_v<
                                  ComparableIndexBase, IndexType>>* = nullptr>
auto operator<=(IndexType lhs, IndexType rhs) -> bool {
  return lhs.index <= rhs.index;
}
template <typename IndexType, typename std::enable_if_t<std::is_base_of_v<
                                  ComparableIndexBase, IndexType>>* = nullptr>
auto operator>(IndexType lhs, IndexType rhs) -> bool {
  return lhs.index > rhs.index;
}
template <typename IndexType, typename std::enable_if_t<std::is_base_of_v<
                                  ComparableIndexBase, IndexType>>* = nullptr>
auto operator>=(IndexType lhs, IndexType rhs) -> bool {
  return lhs.index >= rhs.index;
}

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_INDEX_BASE_H