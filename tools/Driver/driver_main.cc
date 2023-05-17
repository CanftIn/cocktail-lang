#include <cstdlib>
#include <iostream>

#include "llvm/Support/Format.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"

auto main(int argc, char** argv) -> int {
  if (argc < 1) {
    return EXIT_FAILURE;
  }

  std::cout << "this is cocktail driver!!!" << std::endl;

  llvm::raw_ostream& output_stream(llvm::outs());
  llvm::StringRef str = "Hello, LLVM!";
  unsigned width = 20;

  output_stream << "'" << llvm::right_justify(str, width) << "'\n";

  return 0;
}