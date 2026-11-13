// misc.cpp - Chapter 22 miscellaneous rules (part c22p1), part 1.
// Covers: Asmodeus defeat log (R1-R4), teleporter trap (R5-R8),
// chute falls (R9-R15), Hall of Records (R16-R19), monster power (R20),
// skill indexes (R21-R24), pluralisation (R25-R26), countLowBits.
// Currency, X87 arithmetic, WordSort and quest gating are in misc_2.cpp.
//
// UNKNOWN (global): the provided headers do not define FloorState, HallOfRecords,
// HallRecord, or the GameData sub-records (MonsterDef, RaceDef, SpellDef, GuildDef,
// FloorDef, FloorChute) and their field names. Every access to those is isolated in
// the small accessor helpers below, each marked UNKNOWN, so the names can be fixed in
// one place. misc.h also uses HallOfRecords/HallRecord without including the header
// that defines them; that include must be added.
#include "mordor/misc.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

// Applies one chunk of an accumulated chute landing to a member: floor, area,
// landing damage, FALL/HIT events. Used by both walk and TAKE paths.
void applyChuteFall(Ctx& ctx, Party& party, u32 idx, const FloorChute& chute,
                    i32 fromLevel, ChuteFallResult& out);

}  // namespace

// ---------------------------------------------------------------------------
// Asmodeus defeat (R1-R4)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Teleporter trap, trap 7 (R5-R8)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Chute falls (R9-R15)
// ---------------------------------------------------------------------------

namespace {
}  // namespace

// ---------------------------------------------------------------------------
// Hall of Records (R16-R19)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Monster power and skill indexes (R20-R24)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Pluralisation (R25-R26) [c7]
// ---------------------------------------------------------------------------

}  // namespace mordor
