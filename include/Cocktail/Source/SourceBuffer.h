#ifndef COCKTAIL_SOURCE_SOURCE_BUFFER_H
#define COCKTAIL_SOURCE_SOURCE_BUFFER_H

#include <string>
#include <utility>

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Error.h"

namespace Cocktail {

class SourceBuffer {
 public:
  static auto CreateFromText(llvm::Twine text,
                             llvm::StringRef filename = "/text")
      -> llvm::Expected<SourceBuffer>;
  static auto CreateFromFile(llvm::StringRef filename)
      -> llvm::Expected<SourceBuffer>;

  SourceBuffer() = delete;

  SourceBuffer(const SourceBuffer&) = delete;

  SourceBuffer(SourceBuffer&& arg) noexcept;

  ~SourceBuffer();

  [[nodiscard]] auto filename() const -> llvm::StringRef { return filename_; }

  [[nodiscard]] auto text() const -> llvm::StringRef { return text_; }

 private:
  enum class ContentMode {
    Uninitialized,
    MMapped,
    Owned,
  };

  // Constructor for mmapped content.
  explicit SourceBuffer(std::string filename, llvm::StringRef text);
  // Constructor for owned content.
  explicit SourceBuffer(std::string filename, std::string text);

  ContentMode content_mode_;
  std::string filename_;
  std::string text_storage_;
  llvm::StringRef text_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_SOURCE_SOURCE_BUFFER_H