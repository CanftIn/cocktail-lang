#ifndef COCKTAIL_EXPERIMENTAL_SYNTAX_HELPER_H
#define COCKTAIL_EXPERIMENTAL_SYNTAX_HELPER_H

#include <list>

#include "experimental/AST/Declaration.h"

namespace Cocktail {

extern char* input_filename;

void PrintSyntaxError(char* error, int line_num);

void ExecProgram(std::list<Declaration*>* fs);

}  // namespace Cocktail

#endif  // COCKTAIL_EXPERIMENTAL_SYNTAX_HELPER_H