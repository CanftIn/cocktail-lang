#ifndef COCKTAIL_COMMON_CHECK_H
#define COCKTAIL_COMMON_CHECK_H

#include "Cocktail/Common/CheckInternal.h"

namespace Cocktail {

// For example:
//   COCKTAIL_CHECK(is_valid) << "Data is not valid!";
#define COCKTAIL_CHECK(...)                                                 \
  (__VA_ARGS__) ? (void)0                                                   \
                : COCKTAIL_CHECK_INTERNAL_STREAM()                          \
                      << "CHECK failure at " << __FILE__ << ":" << __LINE__ \
                      << ": " #__VA_ARGS__                                  \
                      << Cocktail::Internal::ExitingStream::AddSeparator()

// DCHECK calls CHECK in debug mode, and does nothing otherwise.
#ifndef NDEBUG
#define COCKTAIL_DCHECK(...) COCKTAIL_CHECK(__VA_ARGS__)
#else
#define COCKTAIL_DCHECK(...) COCKTAIL_CHECK(true || (__VA_ARGS__))
#endif

// For example:
//   COCKTAIL_FATAL() << "Unreachable!";
#define COCKTAIL_FATAL()           \
  COCKTAIL_CHECK_INTERNAL_STREAM() \
      << "FATAL failure at " << __FILE__ << ":" << __LINE__ << ": "

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_CHECK_H