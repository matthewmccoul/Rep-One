#pragma once
#include "mordor/types.h"
#include "mordor/state_character.h"

namespace mordor {

// Party.cs:280 BankAnswer: result of a typed bank deposit or withdrawal.
enum class BankAnswer : i32 {
    Ignored = 0,  // input did not parse to a non-zero amount; nothing changed
    Beep = 1,     // refused: insufficient funds, or a deposit of a non-positive amount
    Moved = 2     // gold moved (the reference's unchecked overflow paths still count as Moved)
};


}  // namespace mordor
