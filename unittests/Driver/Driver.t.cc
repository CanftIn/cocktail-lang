#include "Cocktail/Driver/Driver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/SourceMgr.h"

namespace {

using namespace Cocktail;

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::NotNull;
using ::testing::StrEq;

}
