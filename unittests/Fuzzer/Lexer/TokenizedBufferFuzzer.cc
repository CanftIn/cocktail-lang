#include <cstdint>
#include <cstring>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "llvm/ADT/StringRef.h"

namespace Cocktail {

extern "C" auto LLVMFuzzerTestOneInput(const unsigned char* data,
                                       std::size_t size) -> int {
  if (size < 2) {
    return 0;
  }
  uint16_t raw_filename_length;
  std::memcpy(&raw_filename_length, data, 2);
  data += 2;
  size -= 2;
  size_t filename_length = raw_filename_length;

  if (size < filename_length) {
    return 0;
  }
  llvm::StringRef filename(reinterpret_cast<const char*>(data),
                           filename_length);
  data += filename_length;
  size -= filename_length;

  auto source = SourceBuffer::CreateFromText(
      llvm::StringRef(reinterpret_cast<const char*>(data), size), filename);

  DiagnosticEmitter emitter = NullDiagnosticEmitter();

  auto buffer = TokenizedBuffer::Lex(*source, emitter);
  if (buffer.HasErrors()) {
    return 0;
  }

  for (TokenizedBuffer::Token token : buffer.Tokens()) {
    int line_number = buffer.GetLineNumber(token);
    (void)line_number;
    assert(line_number > 0 && "Invalid line number!");
    assert(line_number < INT_MAX && "Invalid line number!");
    int column_number = buffer.GetColumnNumber(token);
    (void)column_number;
    assert(column_number > 0 && "Invalid line number!");
    assert(column_number < INT_MAX && "Invalid line number!");
  }

  return 0;
}

}  // namespace Cocktail