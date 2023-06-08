#ifndef COCKTAIL_COMMON_VLOG_H
#define COCKTAIL_COMMON_VLOG_H

#include "Cocktail/Common/VLogInternal.h"

namespace Cocktail {

// For example:
//   COCKTAIL_VLOG() << "Verbose message";
#define COCKTAIL_VLOG()               \
  (vlog_stream_ == nullptr) ? (void)0 \
                            : COCKTAIL_VLOG_INTERNAL_STREAM(vlog_stream_)

}  // namespace Cocktail

#endif  // COCKTAIL_COMMON_VLOG_H