#ifndef COCKTAIL_COMMON_OSTREAM_H
#define COCKTAIL_COMMON_OSTREAM_H

#include <ostream>

#include "Cocktail/Common/MetaProgramming.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

/// CRTP基类，用于打印类型，子类必须实现Print接口：
/// - auto Print(llvm::raw_ostream& out) -> void;
template <typename DerivedT>
class Printable {
  /// 提供给debugger的简单接口，
  /// `LLVM_DUMP_METHOD` 宏确保只有在调试构建中才会包含这个方法。
  LLVM_DUMP_METHOD void Dump() const {
    static_cast<const DerivedT*>(this)->Print(llvm::errs());
  }

  /// llvm::raw_ostream输出。
  friend auto operator<<(llvm::raw_ostream& out, const DerivedT& obj)
      -> llvm::raw_ostream& {
    obj.Print(out);
    return out;
  }

  /// std::ostream输出。
  friend auto operator<<(std::ostream& out, const DerivedT& obj)
      -> std::ostream& {
    llvm::raw_os_ostream raw_os(out);
    obj.Print(raw_os);
    return out;
  }

  friend auto PrintTo(DerivedT* p, std::ostream* out) -> void {
    *out << static_cast<const void*>(p);
    if (p) {
      *out << " pointing to " << *p;
    }
  }
};

}  // namespace Cocktail

namespace llvm {

/// 注入一个 `operator<<` 重载到 llvm 命名空间，
/// 将 LLVM 类型的 `raw_ostream` 重载映射到 `std::ostream` 重载。
template <typename StreamT, typename ClassT,
          typename = std::enable_if_t<
              std::is_base_of_v<std::ostream, std::decay_t<StreamT>>>,
          typename = std::enable_if_t<
              !std::is_same_v<std::decay_t<ClassT>, raw_ostream>>>
auto operator<<(StreamT& standard_out, const ClassT& value) -> StreamT& {
  raw_os_ostream(standard_out) << value;
  return standard_out;
}

}  // namespace llvm

#endif  // COCKTAIL_COMMON_OSTREAM_H