#ifndef COCKTAIL_COMMON_STRING_HELPERS_H
#define COCKTAIL_COMMON_STRING_HELPERS_H

#include <optional>
#include <string>

#include "Cocktail/Common/Error.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

/// 对源字符串中的转义序列进行反转义。例如将`\n`转换为实际的换行符。
/// \p source为要进行反转义的字符串，
/// \p hashtag_num是表示哈希标签的数量，
/// \p is_block_string表示是否启用块字符串字面量的特殊转义，如 `\<newline>`。
auto UnescapeStringLiteral(llvm::StringRef source, int hashtag_num = 0,
                           bool is_block_string = false)
    -> std::optional<std::string>;

/// 解析源字符串中的块字符串字面量。
auto ParseBlockStringLiteral(llvm::StringRef source, int hashtag_num = 0)
    -> ErrorOr<std::string>;

/// 检查给定的指针是否在给定的`StringRef`范围内，包括与`ref.end()`相等的情况。
auto StringRefContainsPointer(llvm::StringRef ref, const char* ptr) -> bool;

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_STRING_HELPERS_H