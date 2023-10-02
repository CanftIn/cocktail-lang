#ifndef COCKTAIL_PARSE_STATE_H
#define COCKTAIL_PARSE_STATE_H

#include <cstdint>

#include "Cocktail/Common/EnumBase.h"

namespace Cocktail::Parse {

COCKTAIL_DEFINE_RAW_ENUM_CLASS(State, uint8_t) {
#define COCKTAIL_PARSE_STATE(Name) COCKTAIL_RAW_ENUM_ENUMERATOR(Name)
#include "Cocktail/Parse/State.def"
};

class State : public COCKTAIL_ENUM_BASE(State) {
 public:
#define COCKTAIL_PARSE_STATE(Name) COCKTAIL_ENUM_CONSTANT_DECLARATION(Name)
#include "Cocktail/Parse/State.def"
};

#define COCKTAIL_PARSE_STATE(Name) \
  COCKTAIL_ENUM_CONSTANT_DEFINITION(State, Name)
#include "Cocktail/Parse/State.def"

static_assert(sizeof(State) == 1, "State includes padding!");

}  // namespace Cocktail::Parse

#endif  // COCKTAIL_PARSE_STATE_H