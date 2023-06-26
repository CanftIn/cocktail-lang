#include "Cocktail/Driver/Driver.h"

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "Cocktail/Diagnostics/SortingDiagnosticConsumer.h"
#include "Cocktail/Lexer/TokenizedBuffer.h"
#include "Cocktail/Parser/ParseTree.h"
#include "Cocktail/Source/SourceBuffer.h"
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
#define COCKTAIL_SUBCOMMAND(Name, ...) Name,
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
    error_stream_ << "ERROR: No subcommand specified.\n";
    return false;
  }

  llvm::StringRef subcommand_text = args[0];
  llvm::SmallVector<llvm::StringRef, 16> subcommand_args(
      std::next(args.begin()), args.end());

  DiagnosticConsumer* consumer = &ConsoleDiagnosticConsumer();
  std::unique_ptr<SortingDiagnosticConsumer> sorting_consumer;
  // TODO: Figure out command-line support (llvm::cl?), this is temporary.
  if (!subcommand_args.empty() &&
      subcommand_args[0] == "--print-errors=streamed") {
    subcommand_args.erase(subcommand_args.begin());
  } else {
    sorting_consumer = std::make_unique<SortingDiagnosticConsumer>(*consumer);
    consumer = sorting_consumer.get();
  }
  switch (GetSubcommand(subcommand_text)) {
    case Subcommand::Unknown:
      error_stream_ << "ERROR: Unknown subcommand '" << subcommand_text
                    << "'.\n";
      return false;

#define COCKTAIL_SUBCOMMAND(Name, ...) \
  case Subcommand::Name:               \
    return Run##Name##Subcommand(*consumer, subcommand_args);
#include "Cocktail/Driver/Flags.def"
  }
  llvm_unreachable("All subcommands handled!");
}

auto Driver::RunHelpSubcommand(DiagnosticConsumer& /*consumer*/,
                               llvm::ArrayRef<llvm::StringRef> args) -> bool {
  if (!args.empty()) {
    ReportExtraArgs("help", args);
    return false;
  }

  output_stream_ << "List of subcommands:\n\n";

  constexpr llvm::StringLiteral SubcommandsAndHelp[][2] = {
#define COCKTAIL_SUBCOMMAND(Name, Spelling, HelpText) {Spelling, HelpText},
#include "Cocktail/Driver/Flags.def"
  };

  int max_subcommand_width = 0;
  for (const auto* subcommand_and_help : SubcommandsAndHelp) {
    max_subcommand_width = std::max(
        max_subcommand_width, static_cast<int>(subcommand_and_help[0].size()));
  }

  for (const auto* subcommand_and_help : SubcommandsAndHelp) {
    llvm::StringRef subcommand_text = subcommand_and_help[0];
    llvm::StringRef help_text = subcommand_and_help[1];
    output_stream_ << "  "
                   << llvm::left_justify(subcommand_text, max_subcommand_width)
                   << " - " << help_text << "\n";
  }

  output_stream_ << "\n";
  return true;
}

auto Driver::RunDumpTokensSubcommand(DiagnosticConsumer& consumer,
                                     llvm::ArrayRef<llvm::StringRef> args)
    -> bool {
  if (args.empty()) {
    error_stream_ << "ERROR: No input file specified.\n";
    return false;
  }

  llvm::StringRef input_file_name = args.front();
  args = args.drop_front();
  if (!args.empty()) {
    ReportExtraArgs("dump-tokens", args);
    return false;
  }

  auto source = SourceBuffer::CreateFromFile(input_file_name);
  if (!source) {
    error_stream_ << "ERROR: Unable to open input source file: ";
    llvm::handleAllErrors(source.takeError(),
                          [&](const llvm::ErrorInfoBase& ei) {
                            ei.log(error_stream_);
                            error_stream_ << "\n";
                          });
    return false;
  }
  auto tokenized_source = TokenizedBuffer::Lex(*source, consumer);
  consumer.Flush();
  tokenized_source.Print(output_stream_);
  return !tokenized_source.has_errors();
}

auto Driver::RunDumpParseTreeSubcommand(DiagnosticConsumer& consumer,
                                        llvm::ArrayRef<llvm::StringRef> args)
    -> bool {
  if (args.empty()) {
    error_stream_ << "ERROR: No input file specified.\n";
    return false;
  }

  llvm::StringRef input_file_name = args.front();
  args = args.drop_front();
  if (!args.empty()) {
    ReportExtraArgs("dump-parse-tree", args);
    return false;
  }

  auto source = SourceBuffer::CreateFromFile(input_file_name);
  if (!source) {
    error_stream_ << "ERROR: Unable to open input source file: ";
    llvm::handleAllErrors(source.takeError(),
                          [&](const llvm::ErrorInfoBase& ei) {
                            ei.log(error_stream_);
                            error_stream_ << "\n";
                          });
    return false;
  }
  auto tokenized_source = TokenizedBuffer::Lex(*source, consumer);
  auto parse_tree = ParseTree::Parse(tokenized_source, consumer);
  consumer.Flush();
  parse_tree.Print(output_stream_);
  return !tokenized_source.has_errors() && !parse_tree.has_errors();
}

auto Driver::ReportExtraArgs(llvm::StringRef subcommand_text,
                             llvm::ArrayRef<llvm::StringRef> args) -> void {
  error_stream_ << "ERROR: Unexpected additional arguments to the '"
                << subcommand_text << "' subcommand:";
  for (auto arg : args) {
    error_stream_ << " " << arg;
  }

  error_stream_ << "\n";
}

}  // namespace Cocktail