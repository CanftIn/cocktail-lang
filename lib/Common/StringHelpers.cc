#include "Cocktail/Common/StringHelpers.h"

#include "Cocktail/Common/Check.h"
#include "llvm/ADT/StringExtras.h"

namespace Cocktail {

static auto FromHex(char c) -> std::optional<char> {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'A' && c <= 'F') {
    return 10 + c - 'A';
  }
  return std::nullopt;
}

auto UnescapeStringLiteral(llvm::StringRef source)
    -> std::optional<std::string> {
  std::string ret;
  ret.reserve(source.size());
  size_t i = 0;
  while (i < source.size()) {
    char c = source[i];
    switch (c) {
      case '\\':
        ++i;
        if (i == source.size()) {
          return std::nullopt;
        }
        switch (source[i]) {
          case 'n':
            ret.push_back('\n');
            break;
          case 'r':
            ret.push_back('\r');
            break;
          case 't':
            ret.push_back('\t');
            break;
          case '0':
            if (i + 1 < source.size() && llvm::isDigit(source[i + 1])) {
              // \0[0-9] is reserved.
              return std::nullopt;
            }
            ret.push_back('\0');
            break;
          case '"':
            ret.push_back('"');
            break;
          case '\'':
            ret.push_back('\'');
            break;
          case '\\':
            ret.push_back('\\');
            break;
          case 'x': {
            i += 2;
            if (i >= source.size()) {
              return std::nullopt;
            }
            std::optional<char> c1 = FromHex(source[i - 1]);
            std::optional<char> c2 = FromHex(source[i]);
            if (c1 == std::nullopt || c2 == std::nullopt) {
              return std::nullopt;
            }
            ret.push_back(16 * *c1 + *c2);
            break;
          }
          case 'u':
            COCKTAIL_FATAL() << "\\u is not yet supported in string literals";
          default:
            // Unsupported.
            return std::nullopt;
        }
        break;

      case '\t':
        return std::nullopt;

      default:
        ret.push_back(c);
        break;
    }
    ++i;
  }
  return ret;
}

}  // namespace Cocktail