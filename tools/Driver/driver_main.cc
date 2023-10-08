#include <cstdlib>

#include "Cocktail/Driver/Driver.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/InitLLVM.h"

auto main(int argc, char** argv) -> int {
  if (argc < 1) {
    return EXIT_FAILURE;
  }

  llvm::InitLLVM init_llvm(argc, argv);

  // Printing to stderr should flush stdout. This is most noticeable when stderr
  // is piped to stdout.
  llvm::errs().tie(&llvm::outs());

  llvm::SmallVector<llvm::StringRef> args(argv + 1, argv + argc);
  auto fs = llvm::vfs::getRealFileSystem();
  Cocktail::Driver driver(*fs, llvm::outs(), llvm::errs());
  bool success = driver.RunCommand(args);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}