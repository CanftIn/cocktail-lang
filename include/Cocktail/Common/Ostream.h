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

template <typename T>
static constexpr bool HasPrintMethod = Requires<const T, llvm::raw_ostream>(
    [](auto&& t, auto&& out) -> decltype(t.Print(out)) {});

template <typename T, typename = std::enable_if_t<HasPrintMethod<T>>>
auto operator<<(llvm::raw_ostream& out, const T& obj) -> llvm::raw_ostream& {
  obj.Print(out);
  return out;
}

template <typename T, typename = std::enable_if_t<HasPrintMethod<T>>>
auto operator<<(std::ostream& out, const T& obj) -> std::ostream& {
  llvm::raw_os_ostream llvm_os(out);
  obj.Print(llvm_os);
  return out;
}

template <typename T, typename = std::enable_if_t<HasPrintMethod<T>>>
void PrintTo(T* p, std::ostream* out) {
  *out << static_cast<const void*>(p);

  if (p) {
    *out << " pointing to " << *p;
  }
}

// `void PrintID(llvm::raw_ostream& out) const`. Usage:
//
//     out << PrintAsID(obj);
template <typename T>
class PrintAsID {
 public:
  explicit PrintAsID(const T& object) : object_(&object) {}

  friend auto operator<<(llvm::raw_ostream& out, const PrintAsID& self)
      -> llvm::raw_ostream& {
    self.object_->PrintID(out);
    return out;
  }

 private:
  const T* object_;
};

template <typename T>
PrintAsID(const T&) -> PrintAsID<T>;

}  // namespace Cocktail

namespace llvm {

template <typename S, typename T,
          typename = std::enable_if_t<std::is_base_of_v<
              std::ostream, std::remove_reference_t<std::remove_cv_t<S>>>>,
          typename = std::enable_if_t<!std::is_same_v<
              std::remove_reference_t<std::remove_cv_t<T>>, raw_ostream>>>
auto operator<<(S& standard_out, const T& value) -> S& {
  raw_os_ostream(standard_out) << value;
  return standard_out;
}

}  // namespace llvm

#endif  // COCKTAIL_COMMON_OSTREAM_H