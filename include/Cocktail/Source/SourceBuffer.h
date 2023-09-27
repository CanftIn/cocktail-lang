#ifndef COCKTAIL_SOURCE_SOURCE_BUFFER_H
#define COCKTAIL_SOURCE_SOURCE_BUFFER_H

#include <memory>
#include <optional>
#include <string>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/VirtualFileSystem.h"

namespace Cocktail {

class SourceBuffer {
 public:
  // 用于从指定的文件名打开一个文件。
  static auto CreateFromFile(llvm::vfs::FileSystem& fs,
                             llvm::StringRef filename,
                             DiagnosticConsumer& consumer)
      -> std::optional<SourceBuffer>;

  // 用上面的工厂函数来创建一个源缓冲区。
  SourceBuffer() = delete;

  // 返回源文件的名称。
  [[nodiscard]] auto filename() const -> llvm::StringRef { return filename_; }

  // 返回源代码文本的引用。
  [[nodiscard]] auto text() const -> llvm::StringRef {
    return text_->getBuffer();
  }

 private:
  explicit SourceBuffer(std::string filename,
                        std::unique_ptr<llvm::MemoryBuffer> text)
      : filename_(std::move(filename)), text_(std::move(text)) {}

  std::string filename_;                      // 存储源文件的名称。
  std::unique_ptr<llvm::MemoryBuffer> text_;  // 存储源代码文本。
};

}  // namespace Cocktail

#endif  // COCKTAIL_SOURCE_SOURCE_BUFFER_H