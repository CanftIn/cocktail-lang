#ifndef COCKTAIL_DRIVER_DRIVER_H
#define COCKTAIL_DRIVER_DRIVER_H

#include <cstdint>

#include "Cocktail/Diagnostics/DiagnosticEmitter.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

class Driver {
 public:
  Driver() : output_stream_(llvm::outs()), error_stream_(llvm::errs()) {}

  Driver(llvm::raw_ostream& output_stream, llvm::raw_ostream& error_stream)
      : output_stream_(output_stream), error_stream_(error_stream) {}

  auto RunFullCommand(llvm::ArrayRef<llvm::StringRef> args) -> bool;

  auto RunHelpSubcommand(DiagnosticConsumer& consumer,
                         llvm::ArrayRef<llvm::StringRef> args) -> bool;

  auto RunDumpTokensSubcommand(DiagnosticConsumer& consumer,
                               llvm::ArrayRef<llvm::StringRef> args) -> bool;

  auto RunDumpParseTreeSubcommand(DiagnosticConsumer& consumer,
                                  llvm::ArrayRef<llvm::StringRef> args) -> bool;

 private:
  auto ReportExtraArgs(llvm::StringRef subcommand_text,
                       llvm::ArrayRef<llvm::StringRef> args) -> void;

  llvm::raw_ostream& output_stream_;
  llvm::raw_ostream& error_stream_;
};

}  // namespace Cocktail

#endif  // COCKTAIL_DRIVER_DRIVER_H