#ifndef COCKTAIL_COMMON_ENUM_BASE_H
#define COCKTAIL_COMMON_ENUM_BASE_H

#include <type_traits>

#include "Cocktail/Common/Ostream.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail::Internal {

template <typename DerivedT, typename EnumT>
class EnumBase {
 protected:
  using RawEnumType = EnumT;

 public:
  using EnumType = DerivedT;
  using UnderlyingType = std::underlying_type_t<EnumT>;

  constexpr operator RawEnumType() const { return value_; }

  explicit operator bool() const = delete;

  [[nodiscard]] auto name() const -> llvm::StringRef;

  void Print(llvm::raw_ostream& out) const {
    out << static_cast<const EnumType*>(this)->name();
  }

 protected:
  constexpr EnumBase() = default;

  static constexpr auto Create(RawEnumType value) -> EnumType {
    EnumType result;
    result.value_ = value;
    return result;
  }

  constexpr auto AsInt() const -> UnderlyingType {
    return static_cast<UnderlyingType>(value_);
  }

  static constexpr auto FromInt(UnderlyingType value) -> EnumType {
    return Create(static_cast<RawEnumType>(value));
  }

 private:
  static llvm::StringLiteral names[];

  RawEnumType value_;
};

}  // namespace Cocktail::Internal

#define COCKTAIL_DEFINE_RAW_ENUM_CLASS(EnumClassName, UnderlyingType) \
  namespace Internal {                                                \
  enum class EnumClassName##RawEnum : UnderlyingType;                 \
  }                                                                   \
  enum class ::Cocktail::Internal::EnumClassName##RawEnum : UnderlyingType

#define COCKTAIL_RAW_ENUM_ENUMERATOR(Name) Name,

#define COCKTAIL_ENUM_BASE(EnumClassName) \
  COCKTAIL_ENUM_BASE_CRTP(EnumClassName, EnumClassName)

#define COCKTAIL_ENUM_BASE_CRTP(EnumClassName, LocalTypeNameForEnumClass) \
  ::Cocktail::Internal::EnumBase<LocalTypeNameForEnumClass,               \
                                 ::Cocktail::Internal::EnumClassName##RawEnum>

#define COCKTAIL_ENUM_CONSTANT_DECLARATION(Name) static const EnumType Name;

#define COCKTAIL_ENUM_CONSTANT_DEFINITION(EnumClassName, Name) \
  constexpr EnumClassName EnumClassName::Name =                \
      EnumClassName::Create(RawEnumType::Name);

#define COCKTAIL_INLINE_ENUM_CONSTANT_DEFINITION(Name)   \
  static constexpr const typename Base::EnumType& Name = \
      Base::Create(Base::RawEnumType::Name);

#define COCKTAIL_ENUM_NAME_FUNCTION(EnumClassName)                            \
  template <>                                                                 \
  auto                                                                        \
  Internal::EnumBase<EnumClassName, Internal::EnumClassName##RawEnum>::name() \
      const -> llvm::StringRef

#define COCKTAIL_DEFINE_ENUM_CLASS_NAMES(EnumClassName)          \
  template <>                                                    \
  llvm::StringLiteral Internal::EnumBase<                        \
      EnumClassName, Internal::EnumClassName##RawEnum>::names[]; \
                                                                 \
  COCKTAIL_ENUM_NAME_FUNCTION(EnumClassName) {                   \
    return names[static_cast<int>(value_)];                      \
  }                                                              \
                                                                 \
  template <>                                                    \
  llvm::StringLiteral Internal::EnumBase<                        \
      EnumClassName, Internal::EnumClassName##RawEnum>::names[]

#define COCKTAIL_ENUM_CLASS_NAME_STRING(Name) #Name,

#endif  // COCKTAIL_COMMON_ENUM_BASE_H