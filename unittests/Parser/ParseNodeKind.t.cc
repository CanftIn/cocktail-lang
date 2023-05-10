#include "Cocktail/Parser/ParseNodeKind.h"

#include <gtest/gtest.h>

#include <cstring>

#include "llvm/ADT/StringRef.h"

namespace {

using namespace Cocktail;

#define COCKTAIL_PARSE_NODE_KIND(Name)                 \
  TEST(ParseNodeKindTest, Name) {                          \
    EXPECT_EQ(#Name, ParseNodeKind::Name().GetName()); \
  }
#include "Cocktail/Parser/ParseNodeKind.def"

}  // namespace