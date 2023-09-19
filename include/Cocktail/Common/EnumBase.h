#ifndef COCKTAIL_COMMON_ENUM_BASE_H
#define COCKTAIL_COMMON_ENUM_BASE_H

#include <type_traits>

#include "Cocktail/Common/Ostream.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail::Internal {

/// 用CRTP以及X-Macro来生成枚举类的模板类。
template <typename DerivedT, typename EnumT, const llvm::StringLiteral Names[]>
class EnumBase : public Printable<DerivedT> {
 public:
  using RawEnumType = EnumT;  // 用于定义原始的模版类型。
  using EnumType = DerivedT;  // 派生的枚举类型。
  using UnderlyingType =
      std::underlying_type_t<RawEnumType>;  // 原始枚举类型的底层类型。

  /// 允许将枚举类转换为原始的枚举类型，
  constexpr operator RawEnumType() const { return value_; }

  explicit operator bool() const = delete;

  /// 返回枚举的名称。
  [[nodiscard]] auto name() const -> llvm::StringRef { return Names[AsInt()]; }

  /// 打印名称，使用于Printable，必须实现。
  auto Print(llvm::raw_ostream& out) const -> void { out << name(); }

 protected:
  constexpr EnumBase() = default;

  /// 从原始枚举器创建类型。
  static constexpr auto Create(RawEnumType value) -> EnumType {
    EnumType result;
    result.value_ = value;
    return result;
  }

  /// 转换为整数类型。
  constexpr auto AsInt() const -> UnderlyingType {
    return static_cast<UnderlyingType>(value_);
  }

  /// 从底层整数类型转换为枚举类型。
  static constexpr auto FromInt(UnderlyingType value) -> EnumType {
    return Create(static_cast<RawEnumType>(value));
  }

 private:
  RawEnumType value_;
};

}  // namespace Cocktail::Internal

// 创造原始枚举类（不涉及名称）。
#define COCKTAIL_DEFINE_RAW_ENUM_CLASS_NO_NAMES(EnumClassName, UnderlyingType) \
  namespace Internal {                                                         \
  enum class EnumClassName##RawEnum : UnderlyingType;                          \
  }                                                                            \
  enum class Internal::EnumClassName##RawEnum : UnderlyingType

// 创造原始枚举类。
#define COCKTAIL_DEFINE_RAW_ENUM_CLASS(EnumClassName, UnderlyingType) \
  namespace Internal {                                                \
  extern const llvm::StringLiteral EnumClassName##Names[];            \
  }                                                                   \
  COCKTAIL_DEFINE_RAW_ENUM_CLASS_NO_NAMES(EnumClassName, UnderlyingType)

// 在原始枚举类的定义中生成每个枚举值。
#define COCKTAIL_RAW_ENUM_ENUMERATOR(Name) Name,

#define COCKTAIL_ENUM_BASE(EnumClassName) \
  COCKTAIL_ENUM_BASE_CRTP(EnumClassName, EnumClassName, EnumClassName)

#define COCKTAIL_ENUM_BASE_CRTP(EnumClassName, LocalTypeNameForEnumClass, \
                                EnumClassNameForNames)                    \
  ::Cocktail::Internal::EnumBase<LocalTypeNameForEnumClass,               \
                                 Internal::EnumClassName##RawEnum,        \
                                 Internal::EnumClassNameForNames##Names>

// 枚举类体内生成每个值的命名常量声明。
#define COCKTAIL_ENUM_CONSTANT_DECLARATION(Name) static const EnumType Name;

// 枚举类体外定义每个命名常量。
#define COCKTAIL_ENUM_CONSTANT_DEFINITION(EnumClassName, Name) \
  constexpr EnumClassName EnumClassName::Name =                \
      EnumClassName::Create(RawEnumType::Name);

#define COCKTAIL_INLINE_ENUM_CONSTANT_DEFINITION(Name)   \
  static constexpr const typename Base::EnumType& Name = \
      Base::Create(Base::RawEnumType::Name);

// 在 `.cc` 文件中为枚举类开始定义每个枚举器的常量名数组。
#define COCKTAIL_DEFINE_ENUM_CLASS_NAMES(EnumClassName) \
  constexpr llvm::StringLiteral Internal::EnumClassName##Names[]

#define COCKTAIL_ENUM_CLASS_NAME_STRING(Name) #Name,

#endif  // COCKTAIL_COMMON_ENUM_BASE_H