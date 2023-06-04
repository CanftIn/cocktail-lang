#include <cstdlib>
#include <iostream>

#include "Cocktail/Driver/Driver.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

auto main(int argc, char** argv) -> int {
  if (argc < 1) {
    return EXIT_FAILURE;
  }

  std::cout << "This is cocktail driver!!!\n";

  llvm::SmallVector<llvm::StringRef, 16> args(argv + 1, argv + argc);
  Cocktail::Driver driver;
  bool success = driver.RunFullCommand(args);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}