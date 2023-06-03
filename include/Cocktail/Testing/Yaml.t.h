#ifndef COCKTAIL_TESTING_YAML_T_H
#define COCKTAIL_TESTING_YAML_T_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <variant>

#include "llvm/ADT/StringRef.h"

namespace Cocktail::Testing::Yaml {

struct EmptyComparable {
  friend auto operator==(const EmptyComparable& /*unused*/,
                         const EmptyComparable& /*unused*/) -> bool {
    return true;
  }

  friend auto operator!=(const EmptyComparable& /*unused*/,
                         const EmptyComparable& /*unused*/) -> bool {
    return false;
  }
};

struct Value;
struct NullValue : EmptyComparable {};
using ScalarValue = std::string;
using MappingValue = std::vector<std::pair<Value, Value>>;
using SequenceValue = std::vector<Value>;
struct AliasValue : EmptyComparable {};
struct ErrorValue : EmptyComparable {};

struct Value : std::variant<NullValue, ScalarValue, MappingValue, SequenceValue,
                            AliasValue, ErrorValue> {
  using variant::variant;

  friend auto operator<<(std::ostream& os, const Value& value) -> std::ostream&;

  static auto FromText(llvm::StringRef text) -> SequenceValue;
};

template <typename T>
auto DescribeMatcher(::testing::Matcher<T> matcher) -> std::string {
  std::ostringstream out;
  matcher.DescribeTo(&out);
  return out.str();
}

MATCHER_P(Mapping, contents,
          "is mapping that " + DescribeMatcher<SequenceValue>(contents)) {
  ::testing::Matcher<SequenceValue> contents_matcher = contents;

  if (auto* map = std::get_if<SequenceValue>(&arg)) {
    return contents_matcher.MatchAndExplain(*map, result_listener);
  }

  *result_listener << "which is not a sequence";
  return false;
}

MATCHER_P(Scalar, value,
          "has scalar value " + ::testing::PrintToString(value)) {
  ::testing::Matcher<ScalarValue> value_matcher = value;

  if (auto* map = std::get_if<ScalarValue>(&arg)) {
    return value_matcher.MatchAndExplain(*map, result_listener);
  }

  *result_listener << "which is not a scalar";
  return false;
}

}  // namespace Cocktail::Testing::Yaml

#endif  // COCKTAIL_TESTING_YAML_T_H