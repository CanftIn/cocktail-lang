#ifndef COCKTAIL_COMMON_CHECK_H
#define COCKTAIL_COMMON_CHECK_H

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

#define CHECK(CONDITION)                                    \
  if (!(CONDITION)) {                                       \
    llvm::sys::PrintStackTrace(llvm::errs());               \
    llvm::report_fatal_error("CHECK failure: " #CONDITION); \
  }

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_CHECK_H