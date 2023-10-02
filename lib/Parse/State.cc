#include "Cocktail/Parse/State.h"

namespace Cocktail::Parse {

COCKTAIL_DEFINE_ENUM_CLASS_NAMES(State) = {
#define COCKTAIL_PARSE_STATE(Name) COCKTAIL_ENUM_CLASS_NAME_STRING(Name)
#include "Cocktail/Parse/State.def"
};

}  // namespace Cocktail::Parse