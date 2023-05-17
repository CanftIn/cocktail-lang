#ifndef COCKTAIL_DRIVER_DRIVER_H
#define COCKTAIL_DRIVER_DRIVER_H

#include <cstdint>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

class Driver {
 public:
  Driver() : output_stream(llvm::outs()), error_stream(llvm::errs()) {}

  Driver(llvm::raw_ostream& output_stream, llvm::raw_ostream& error_stream)
      : output_stream(output_stream), error_stream(error_stream) {}

  auto RunFullCommand(llvm::ArrayRef<llvm::StringRef> args) -> bool;

  auto RunHelpSubcommand(llvm::ArrayRef<llvm::StringRef> args) -> bool;

  auto RunDumpTokensSubcommand(llvm::ArrayRef<llvm::StringRef> args) -> bool;

  auto RunDumpParseTreeSubcommand(llvm::ArrayRef<llvm::StringRef> args) -> bool;

 private:
  auto ReportExtraArgs(llvm::StringRef subcommand_text,
                       llvm::ArrayRef<llvm::StringRef> args) -> void;

  llvm::raw_ostream& output_stream;
  llvm::raw_ostream& error_stream;
};

}  // namespace Cocktail

#endif  // COCKTAIL_DRIVER_DRIVER_H