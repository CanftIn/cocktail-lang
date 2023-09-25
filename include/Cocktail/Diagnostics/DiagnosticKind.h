#ifndef COCKTAIL_DIAGNOSTIC_DIAGNOSTIC_KIND_H
#define COCKTAIL_DIAGNOSTIC_DIAGNOSTIC_KIND_H

#include "Cocktail/Common/EnumBase.h"

namespace Cocktail {

COCKTAIL_DEFINE_RAW_ENUM_CLASS(DiagnosticKind, uint16_t) {
#define COCKTAIL_DIAGNOSTIC_KIND(Name) COCKTAIL_RAW_ENUM_ENUMERATOR(Name)
#include "Cocktail/Diagnostics/DiagnosticKind.def"
};

/// 用于表示工具链提供的所有诊断类型。
class DiagnosticKind : public COCKTAIL_ENUM_BASE(DiagnosticKind) {
 public:
#define COCKTAIL_DIAGNOSTIC_KIND(Name) COCKTAIL_ENUM_CONSTANT_DECLARATION(Name)
#include "Cocktail/Diagnostics/DiagnosticKind.def"
};

#define COCKTAIL_DIAGNOSTIC_KIND(Name) \
  COCKTAIL_ENUM_CONSTANT_DEFINITION(DiagnosticKind, Name)
#include "Cocktail/Diagnostics/DiagnosticKind.def"

// 使用静态断言来确保DiagnosticKind的大小是2字节，以确保没有填充。
static_assert(sizeof(DiagnosticKind) == 2, "DiagnosticKind includes padding!");

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTIC_DIAGNOSTIC_KIND_H