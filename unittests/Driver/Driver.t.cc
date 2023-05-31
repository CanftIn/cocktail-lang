#include "Cocktail/Driver/Driver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Cocktail/Common/Yaml.t.h"
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
namespace Yaml = Cocktail::Testing::Yaml;

class RawTestOstream : public llvm::raw_ostream {
  std::string buffer;

  void write_impl(const char* ptr, size_t size) override {
    buffer.append(ptr, ptr + size);
  }

  [[nodiscard]] auto current_pos() const -> uint64_t override {
    return buffer.size();
  }

 public:
  ~RawTestOstream() override {
    flush();
    if (!buffer.empty()) {
      ADD_FAILURE() << "Unchecked output:\n" << buffer;
    }
  }

  auto TakeStr() -> std::string {
    flush();
    std::string result = std::move(buffer);
    buffer.clear();
    return result;
  }
};

TEST(DriverTest, FullCommandErrors) {
  RawTestOstream test_output_stream;
  RawTestOstream test_error_stream;
  Driver driver(test_output_stream, test_error_stream);

  EXPECT_FALSE(driver.RunFullCommand({}));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));

  EXPECT_FALSE(driver.RunFullCommand({"foo"}));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));

  EXPECT_FALSE(driver.RunFullCommand({"foo --bar --baz"}));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));
}

TEST(DriverTest, Help) {
  RawTestOstream test_output_stream;
  RawTestOstream test_error_stream;
  Driver driver = Driver(test_output_stream, test_error_stream);

  EXPECT_TRUE(driver.RunHelpSubcommand({}));
  EXPECT_THAT(test_error_stream.TakeStr(), StrEq(""));
  auto help_text = test_output_stream.TakeStr();

#define CARBON_SUBCOMMAND(Name, Spelling, ...) \
  EXPECT_THAT(help_text, HasSubstr(Spelling));
#include "Cocktail/Driver/Flags.def"

  EXPECT_TRUE(driver.RunFullCommand({"help"}));
  EXPECT_THAT(test_error_stream.TakeStr(), StrEq(""));
  EXPECT_THAT(test_output_stream.TakeStr(), StrEq(help_text));
}

TEST(DriverTest, HelpErrors) {
  RawTestOstream test_output_stream;
  RawTestOstream test_error_stream;
  Driver driver = Driver(test_output_stream, test_error_stream);

  EXPECT_FALSE(driver.RunHelpSubcommand({"foo"}));
  EXPECT_THAT(test_output_stream.TakeStr(), StrEq(""));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));

  EXPECT_FALSE(driver.RunHelpSubcommand({"help"}));
  EXPECT_THAT(test_output_stream.TakeStr(), StrEq(""));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));

  EXPECT_FALSE(driver.RunHelpSubcommand({"--xyz"}));
  EXPECT_THAT(test_output_stream.TakeStr(), StrEq(""));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));
}

auto CreateTestFile(llvm::StringRef text) -> std::string {
  int fd = -1;
  llvm::SmallString<1024> path;
  auto ec = llvm::sys::fs::createTemporaryFile("test_file", ".txt", fd, path);
  if (ec) {
    llvm::report_fatal_error(llvm::Twine("Failed to create temporary file: ") +
                             ec.message());
  }

  llvm::raw_fd_ostream s(fd, /*shouldClose=*/true);
  s << text;
  s.close();

  return path.str().str();
}

TEST(DriverTest, DumpTokens) {
  RawTestOstream test_output_stream;
  RawTestOstream test_error_stream;
  Driver driver = Driver(test_output_stream, test_error_stream);

  auto test_file_path = CreateTestFile("Hello World");
  EXPECT_TRUE(driver.RunDumpTokensSubcommand({test_file_path}));
  EXPECT_THAT(test_error_stream.TakeStr(), StrEq(""));
  auto tokenized_text = test_output_stream.TakeStr();

  EXPECT_THAT(Yaml::Value::FromText(tokenized_text),
              ElementsAre(Yaml::MappingValue{
                  {"token", Yaml::MappingValue{{"index", "0"},
                                               {"kind", "Identifier"},
                                               {"line", "1"},
                                               {"column", "1"},
                                               {"indent", "1"},
                                               {"spelling", "Hello"},
                                               {"identifier", "0"}}},
                  {"token", Yaml::MappingValue{{"index", "1"},
                                               {"kind", "Identifier"},
                                               {"line", "1"},
                                               {"column", "7"},
                                               {"indent", "1"},
                                               {"spelling", "World"},
                                               {"identifier", "1"}}}}));

  // Check that the subcommand dispatch works.
  EXPECT_TRUE(driver.RunFullCommand({"dump-tokens", test_file_path}));
  EXPECT_THAT(test_error_stream.TakeStr(), StrEq(""));
  EXPECT_THAT(test_output_stream.TakeStr(), StrEq(tokenized_text));
}

TEST(DriverTest, DumpTokenErrors) {
  RawTestOstream test_output_stream;
  RawTestOstream test_error_stream;
  Driver driver = Driver(test_output_stream, test_error_stream);

  EXPECT_FALSE(driver.RunDumpTokensSubcommand({}));
  EXPECT_THAT(test_output_stream.TakeStr(), StrEq(""));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));

  EXPECT_FALSE(driver.RunDumpTokensSubcommand({"--xyz"}));
  EXPECT_THAT(test_output_stream.TakeStr(), StrEq(""));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));

  EXPECT_FALSE(driver.RunDumpTokensSubcommand({"/not/a/real/file/name"}));
  EXPECT_THAT(test_output_stream.TakeStr(), StrEq(""));
  EXPECT_THAT(test_error_stream.TakeStr(), HasSubstr("ERROR"));
}

}  // namespace
