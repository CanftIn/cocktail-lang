#include <cstdlib>
#include <iostream>

auto main(int argc, char** argv) -> int {
  if (argc < 1) {
    return EXIT_FAILURE;
  }

  std::cout << "this is cocktail driver!!!" << std::endl;

  return 0;
}