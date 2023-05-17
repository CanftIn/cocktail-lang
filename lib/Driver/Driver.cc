#include "Cocktail/Driver/Driver.h"

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseTree.h"
#include "Cocktail/SourceBuffer.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"

namespace Cocktail {

namespace {

enum class Subcommand {
#define COCKTAIL_SUBCOMMAND(NAME, ...) NAME,
#include "Cocktail/Driver/Flags.def"
  Unknown,
};

auto GetSubcommand(llvm::StringRef name) -> Subcommand {
  return llvm::StringSwitch<Subcommand>(name)
#define COCKTAIL_SUBCOMMAND(Name, Spelling, ...) \
  .Case(Spelling, Subcommand::Name)
#include "Cocktail/Driver/Flags.def"
      .Default(Subcommand::Unknown);
}

}  // namespace

auto Driver::RunFullCommand(llvm::ArrayRef<llvm::StringRef> args) -> bool {
  if (args.empty()) {
    error_stream << "ERROR: No subcommand specified.\n";
    return false;
  }

  llvm::StringRef subcommand_text = args.front();
  llvm::SmallVector<llvm::StringRef, 16> subcommand_args(
      std::next(args.begin()), args.end());
  switch (GetSubcommand(subcommand_text)) {
    case Subcommand::Unknown:
      error_stream << "ERROR: Unknown subcommand '" << subcommand_text
                   << "'.\n";
      return false;

#define COCKTAIL_SUBCOMMAND(Name, ...) \
  case Subcommand::Name:               \
    return Run##Name##Subcommand(subcommand_args);
#include "Cocktail/Driver/Flags.def"
  }

  llvm_unreachable("All subcommand handled!.");
}

auto Driver::RunHelpSubcommand(llvm::ArrayRef<llvm::StringRef> args) -> bool {
  if (!args.empty()) {
    ReportExtraArgs("help", args);
    return false;
  }

  output_stream << "List of subcommands:\n\n";

  constexpr llvm::StringLiteral SubcommandsAndHelp[][2] = {
#define COCKTAIL_SUBCOMMAND(Name, Spelling, HelpText) {Spelling, HelpText},
#include "Cocktail/Driver/Flags.def"
  };

  int max_subcommand_width = 0;
  for (auto subcommand_and_help : SubcommandsAndHelp) {
    max_subcommand_width = std::max(
        max_subcommand_width, static_cast<int>(subcommand_and_help[0].size()));
  }

  for (auto subcommand_and_help : SubcommandsAndHelp) {
    llvm::StringRef subcommand_text = subcommand_and_help[0];
    llvm::StringRef help_text = subcommand_and_help[1];
    output_stream << "  "
                  << llvm::left_justify(subcommand_text, max_subcommand_width)
                  << " - " << help_text << "\n";
  }

  output_stream << "\n";
  return true;
}

auto Driver::RunDumpTokensSubcommand(llvm::ArrayRef<llvm::StringRef> args)
    -> bool {
  if (args.empty()) {
    error_stream << "ERROR: No input file specified.\n";
    return false;
  }
  llvm::StringRef input_file_path = args.front();
  args = args.drop_front();
  if (!args.empty()) {
    ReportExtraArgs("dump-tokens", args);
    return false;
  }

  auto source = SourceBuffer::CreateFromFile(input_file_path);
  if (!source) {
    error_stream << "ERROR: Unable to open input source file: ";
    llvm::handleAllErrors(source.takeError(),
                          [&](const llvm::ErrorInfoBase& error) {
                            error.log(error_stream);
                            error_stream << "\n";
                          });
    return false;
  }

  auto tokenized_source = TokenizedBuffer::Lex(*source);
  tokenized_source.Print(output_stream);
  return !tokenized_source.HasErrors();
}

auto Driver::RunDumpParseTreeSubcommand(llvm::ArrayRef<llvm::StringRef> args)
    -> bool {
  if (args.empty()) {
    error_stream << "ERROR: No input file specified.\n";
    return false;
  }

  llvm::StringRef input_file_path = args.front();
  args = args.drop_front();
  if (!args.empty()) {
    ReportExtraArgs("dump-parse-tree", args);
    return false;
  }

  auto source = SourceBuffer::CreateFromFile(input_file_path);
  if (!source) {
    error_stream << "ERROR: Unable to open input source file: ";
    llvm::handleAllErrors(source.takeError(),
                          [&](const llvm::ErrorInfoBase& error) {
                            error.log(error_stream);
                            error_stream << "\n";
                          });
    return false;
  }

  auto tokenized_source = TokenizedBuffer::Lex(*source);
  auto parse_tree = ParseTree::Parse(tokenized_source);
  parse_tree.Print(output_stream);
  return !tokenized_source.HasErrors() && !parse_tree.HasErrors();
}

auto Driver::ReportExtraArgs(llvm::StringRef subcommand_text,
                             llvm::ArrayRef<llvm::StringRef> args) -> void {
  error_stream << "ERROR: Unexpected additional arguments to the '"
               << subcommand_text << "' subcommand:";
  for (llvm::StringRef arg : args) {
    error_stream << " " << arg;
  }
  error_stream << "\n";
}

}  // namespace Cocktail