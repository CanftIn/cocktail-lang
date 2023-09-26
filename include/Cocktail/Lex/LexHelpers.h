#ifndef COCKTAIL_LEX_LEX_HELPERS_H
#define COCKTAIL_LEX_LEX_HELPERS_H

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"

namespace Cocktail::Lex {

// 检查是否可以对给定的文本（字符串）进行整数词法分析（即将字符串转换为整数）。
// 使用`llvm::getAsInteger`进行解析可能在大整数值上表现得非常慢。
// 因此函数内部设置了一个数值大小的限制。
auto CanLexInteger(DiagnosticEmitter<const char*>& emitter,
                   llvm::StringRef text) -> bool;

}  // namespace Cocktail

#endif  // COCKTAIL_LEX_LEX_HELPERS_H