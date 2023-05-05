#include "Cocktail/SourceBuffer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <system_error>

#include "llvm/ADT/ScopeExit.h"

namespace Cocktail {

auto SourceBuffer::CreateFromText(llvm::Twine text, llvm::StringRef filename)
    -> SourceBuffer {
  return SourceBuffer(filename, text.str());
}

static auto ErrnoToError(int errno_value) -> llvm::Error {
  return llvm::errorCodeToError(
      std::error_code(errno_value, std::system_category()));
}

auto SourceBuffer::CreateFromFile(llvm::StringRef filename)
    -> llvm::Expected<SourceBuffer> {
  SourceBuffer buffer(filename);

  errno = 0;
  int file_descriptor = open(buffer.filename_.c_str(), O_RDONLY);
  if (file_descriptor == -1)
    return ErrnoToError(errno);

  auto closer =
      llvm::make_scope_exit([file_descriptor] { close(file_descriptor); });

  struct stat stat_buffer = {};
  errno = 0;
  if (fstat(file_descriptor, &stat_buffer) == -1)
    return ErrnoToError(errno);

  int64_t size = stat_buffer.st_size;
  if (size == 0) {
    return {std::move(buffer)};
  }

  errno = 0;
  void* mapped_text = mmap(nullptr, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE,
                           file_descriptor, /*offset=*/0);
  if (mapped_text == MAP_FAILED)
    return ErrnoToError(errno);

  errno = 0;
  closer.release();
  if (close(file_descriptor) == -1) {
    munmap(mapped_text, size);
    return ErrnoToError(errno);
  }

  buffer.text_ = llvm::StringRef(static_cast<const char*>(mapped_text), size);
  assert(!buffer.text_.empty() &&
         "Must not have an empty text when we have mapped data from a file!");
  return {std::move(buffer)};
}

SourceBuffer::~SourceBuffer() {
  if (!is_string_rep_) {
    using StringType = decltype(string_storage_);
    string_storage_.~StringType();
    return;
  }

  if (!text_.empty()) {
    errno = 0;
    int result =
        munmap(const_cast<void*>((const void*)text_.data()), text_.size());
    (void)result;
    assert(result != -1 && "Unmapping text failed!");
  }
}

}  // namespace Cocktail