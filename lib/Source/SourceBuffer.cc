#include "Cocktail/Source/SourceBuffer.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <limits>
#include <system_error>

#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/Error.h"

namespace Cocktail {

static auto CheckContentSize(int64_t size) -> llvm::Error {
  if (size < std::numeric_limits<int32_t>::max()) {
    return llvm::Error::success();
  }
  return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                 "Input too large!");
}

auto SourceBuffer::CreateFromText(llvm::Twine text, llvm::StringRef filename)
    -> llvm::Expected<SourceBuffer> {
  std::string buffer = text.str();
  auto size_check = CheckContentSize(buffer.size());
  if (size_check) {
    return std::move(size_check);
  }
  return SourceBuffer(filename.str(), std::move(buffer));
}

static auto ErrnoToError(int errno_value) -> llvm::Error {
  return llvm::errorCodeToError(
      std::error_code(errno_value, std::generic_category()));
}

auto SourceBuffer::CreateFromFile(llvm::StringRef filename)
    -> llvm::Expected<SourceBuffer> {
  std::string filename_str = filename.str();

  errno = 0;
  int file_descriptor = open(filename_str.c_str(), O_RDONLY);
  if (file_descriptor == -1) {
    return ErrnoToError(errno);
  }

  auto closer =
      llvm::make_scope_exit([file_descriptor] { close(file_descriptor); });

  struct stat stat_buffer = {};
  errno = 0;
  if (fstat(file_descriptor, &stat_buffer) == -1) {
    return ErrnoToError(errno);
  }

  int64_t size = stat_buffer.st_size;
  if (size == 0) {
    return SourceBuffer(std::move(filename_str), std::string());
  }
  auto size_check = CheckContentSize(size);
  if (size_check) {
    return std::move(size_check);
  }

  errno = 0;
  void* mapped_text = mmap(nullptr, size, PROT_READ,
#if defined(__Linux__)
                           MAP_PRIVATE | MAP_POPULATE,
#else
                           MAP_PRIVATE,
#endif
                           file_descriptor, /*offset=*/0);
  if (mapped_text == MAP_FAILED) {
    return ErrnoToError(errno);
  }

  errno = 0;
  closer.release();
  if (close(file_descriptor) == -1) {
    munmap(mapped_text, size);
    return ErrnoToError(errno);
  }

  return SourceBuffer(
      std::move(filename_str),
      llvm::StringRef(static_cast<const char*>(mapped_text), size));
}

SourceBuffer::SourceBuffer(SourceBuffer&& arg) noexcept
    // Sets Uninitialized to ensure the input doesn't release mmapped data.
    : content_mode(std::exchange(arg.content_mode, ContentMode::Uninitialized)),
      filename(std::move(arg.filename)),
      text_storage(std::move(arg.text_storage)),
      text(content_mode == ContentMode::Owned ? text_storage : arg.text) {}

SourceBuffer::SourceBuffer(std::string filename, std::string text)
    : content_mode(ContentMode::Owned),
      filename(std::move(filename)),
      text_storage(std::move(text)),
      text(text_storage) {}

SourceBuffer::SourceBuffer(std::string filename, llvm::StringRef text)
    : content_mode(ContentMode::MMapped),
      filename(std::move(filename)),
      text(text) {
  assert((!text.empty()) &&
         "Must not have an empty text when we have mapped data from a file!");
}

SourceBuffer::~SourceBuffer() {
  if (content_mode == ContentMode::MMapped) {
    errno = 0;
    int result = munmap(
        const_cast<void*>(static_cast<const void*>(text.data())), text.size());
    assert((result != -1) && "Unmapping text failed!");
  }
}

}  // namespace Cocktail