#pragma once
// One shared copy of helpers that several modules had each written for themselves.
#include "mordor/types.h"
#include "mordor/game_data.h"
#include "mordor/lookup.h"
#include "mordor/death1a.h"
#include "mordor/clock.h"
#include "mordor/loot1a.h"
#include "mordor/progression1a.h"
#include "mordor/monsters1b.h"
#include "mordor/combat3.h"
#include "mordor/combat2a.h"
#include "mordor/combat1b.h"
#include "mordor/combat1a.h"
#include "mordor/store_prices.h"
#include "mordor/progression2b.h"
#include "mordor/progression2.h"
#include "mordor/partyb.h"
#include "mordor/items1a.h"
#include "mordor/guilds1.h"
#include "mordor/city1.h"
#include "mordor/items1b.h"
#include "mordor/state_common.h"
#include "mordor/game.h"
#include "mordor/casting1.h"
#include "mordor/access.h"
#include "mordor/features.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_creature.h"
#include "mordor/state_companion.h"
#include "mordor/state_character.h"
#include "mordor/rng.h"
#include "mordor/monsters1a.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"

namespace mordor {
u32 uidx(i32 i);
Str seerJoinWords(const Vec<Str>& w, size_t from);
Str bufferInt(i64 v);
const Character& memberAt(const Party& p, i32 i);
bool isDigits(const Str& t);
bool swapSlotInRange(i32 slot);
Str joinFrom(const Vec<Str>& w, std::size_t from);
Str featureStateOr(const Session& s, const Str& key, const Str& dflt);
bool parseInt(const Str& t, i32& out);
Str toStr(i64 v);
bool confParseIndex(const Str& w, i32& out);
enum class CurrencyFraction {
  FourDigitsAlways,  // "12.0000"
  FourDigitsIfAny,   // "0.5000"; whole gold alone when the fraction is zero
  TrimmedIfAny       // "0.5"; trailing zeros dropped; whole gold alone when the fraction is zero
};
// Signed decimal text of a Currency held in 1/10000 gold.
Str formatCurrencyText(Currency units, CurrencyFraction fraction, bool goldSuffix);
Vec<Str> splitWords(const Str& text);
// Maps a pen/alignment index to its label. 0 = Good, 1 = Neutral, 2 = Evil.
// Any other value returns outOfRange.
Str alignLabel(i32 align, Str outOfRange);

// Adds `days` to ch.ageDays, floored at 0. A non-null announceCtx announces the change; nullptr is silent.
void ageCharacterDays(Character& ch, i32 days, Ctx* announceCtx);

}  // namespace mordor
