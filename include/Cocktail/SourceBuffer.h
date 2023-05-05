#ifndef COCKTAIL_SOURCEBUFFER_H
#define COCKTAIL_SOURCEBUFFER_H

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
      -> SourceBuffer;

  static auto CreateFromFile(llvm::StringRef filename)
      -> llvm::Expected<SourceBuffer>;

  SourceBuffer() = delete;

  SourceBuffer(const SourceBuffer&) = delete;

  SourceBuffer(SourceBuffer&& arg)
      : filename_(std::move(arg.filename_)),
        text_(arg.text_),
        is_string_rep_(arg.is_string_rep_) {
    if (!arg.is_string_rep_) {
      arg.text_ = llvm::StringRef();
      return;
    }

    new (&string_storage_) std::string(std::move(arg.string_storage_));
    text_ = string_storage_;
  }

  ~SourceBuffer();

  llvm::StringRef Filename() const { return filename_; }

  llvm::StringRef Text() const { return text_; }

 private:
  std::string filename_;

  llvm::StringRef text_;

  bool is_string_rep_;

  union {
    std::string string_storage_;
  };

  explicit SourceBuffer(llvm::StringRef fake_filename, std::string buffer_text)
      : filename_(fake_filename),
        is_string_rep_(true),
        string_storage_(buffer_text) {
    text_ = string_storage_;
  }

  explicit SourceBuffer(llvm::StringRef filename)
      : filename_(filename.str()), text_(), is_string_rep_(false) {}
};

}  // namespace Cocktail

#endif  // COCKTAIL_SOURCEBUFFER_H