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

// UNKNOWN: FloorState wall grid is not available to this module. Stub returns rock
// for every square, so the teleporter fails safe (no teleport) until wired up.
u32 floorWallWord(const FloorState& floor, i32 x, i32 y) {
    (void)floor;
    (void)x;
    (void)y;
    return kWallMaskRock;
}

// UNKNOWN: FloorDef area layout not available (FIXED: FloorDef has no areaGrid field).
// Coordinates are 1-based on a 30x30 map. Returns -1 off-map, 0 otherwise.
i32 floorAreaAt(const GameData& data, i32 level, i32 x, i32 y) {
    if (level < 0 || static_cast<u32>(level) >= data.floors.size()) return -1;
    if (x < kTeleportAxisMin || x > kTeleportAxisMax) return -1;
    if (y < kTeleportAxisMin || y > kTeleportAxisMax) return -1;
    (void)data.floors[static_cast<u32>(level)];  // FIXED: no areaGrid; area unknown
    return 0;
}

// UNKNOWN: RaceDef.size assumed (SRD: RaceData.Size).
i32 raceSizeOf(const GameData& data, Race race) {
    return data.races[static_cast<u32>(race)].size;
}

// UNKNOWN: MonsterDef looked up by monster id, fields att/str/hits/dex/guildLevel/
// specialBits/specialPropertyFlags assumed.
const MonsterDef& monsterDefAt(const GameData& data, i32 monsterId) {
    return data.monsters[static_cast<u32>(monsterId)];
}

// FIXED: HallOfRecords.records holds plain integer values, not HallRecord entries, so a
// record is rebuilt from its stored value. HallRecord has no holderIsPlayer or date
// field, so those are not kept.
HallRecord hallSlot(const HallOfRecords& hall, RecordKind kind) {
    HallRecord rec{};
    rec.value = hall.records[static_cast<u32>(kind)];
    return rec;
}

// R16-R19 display names for the record kinds.
const char* recordKindName(RecordKind kind) {
    switch (kind) {
        case RecordKind::Strength: return "Strength";
        case RecordKind::Intelligence: return "Intelligence";
        case RecordKind::Wisdom: return "Wisdom";
        case RecordKind::Constitution: return "Constitution";
        case RecordKind::Charisma: return "Charisma";
        case RecordKind::Dexterity: return "Dexterity";
        case RecordKind::Deadliest: return "Deadliest";
        case RecordKind::MostExperience: return "Most Experience";
        case RecordKind::Wealthiest: return "Wealthiest";
        case RecordKind::Fighting: return "Fighting";
        case RecordKind::Magic: return "Magic";
        case RecordKind::Thieving: return "Thieving";
    }
    return "Record";
}

// R20: shared monster power formula, used by both monster paths.
i64 powerFromFields(i32 att, i32 strength, i32 hits, i32 dex, i32 guildLevel,
                    u32 specialFlags, u32 spellFlags) {
    // first = round_even(Att*2 + STR*10 + Hits/10.0)
    i64 first = roundHalfEven(static_cast<double>(att) * 2.0 +
                              static_cast<double>(strength) * 10.0 +
                              static_cast<double>(hits) / 10.0);
    // power = first + DEX*2 + GuildLevel + 75*count(special 0..13) + 100*count(spell 0..17)
    // UNKNOWN: Creature has no dedicated spell-bit field; specialBits is taken as the
    // special-attack bits and specialPropertyFlags as the spell bits.
    return first + static_cast<i64>(dex) * 2 + static_cast<i64>(guildLevel) +
           kPowerSpecialWeight * countLowBits(specialFlags, kPowerSpecialBits) +
           kPowerSpellWeight * countLowBits(spellFlags, kPowerSpellBits);
}

// R25 helper: true if s ends with the given suffix (exact case).
bool endsWith(const Str& s, const char* suffix) {
    i32 sl = 0;
    while (suffix[sl] != '\0') ++sl;
    i32 n = static_cast<i32>(s.size());
    if (n < sl) return false;
    for (i32 i = 0; i < sl; ++i) {
        if (s[static_cast<u32>(n - sl + i)] != suffix[i]) return false;
    }
    return true;
}

// Removes the last k characters.
Str dropLast(const Str& s, i32 k) {
    return s.substr(0, static_cast<u32>(static_cast<i32>(s.size()) - k));
}

// Applies one chunk of an accumulated chute landing to a member: floor, area,
// landing damage, FALL/HIT events. Used by both walk and TAKE paths.
void applyChuteFall(Ctx& ctx, Party& party, u32 idx, const FloorChute& chute,
                    i32 fromLevel, ChuteFallResult& out);

}  // namespace

// ---------------------------------------------------------------------------
// Asmodeus defeat (R1-R4)
// ---------------------------------------------------------------------------

// R1: party or lone wording depends on whether the player is in a party at defeat.
// UNKNOWN: "in a party" taken as more than one member.
bool inPartyAtDefeat(const Party& party) {
    return party.size > 1;
}

// R1: Asmodeus.Ending. UNKNOWN: ending body text is not in the chunks; placeholder
// wording below. The five form-selection sites (R2) are UNKNOWN and not modelled.
void sayAsmodeusEnding(Ctx& ctx, bool inAParty) {
    ctx.say("asmodeus.ending",
            inAParty ? "Asmodeus is destroyed. Your party has won the day."
                     : "Asmodeus is destroyed. You stand alone in victory.",
            {inAParty ? 1 : 0}, {});
}

// Asmodeus log line [c1]: LogLine = inAParty ? PartyBeatAsmodeus(name) : AloneBeatAsmodeus(name).
// Quirk: lone wording is "single handedly defeated Asmodeus"; party wording is
// "was part of the party that defeated" him. UNKNOWN: full sentence framing.
Str asmodeusLogLine(const Str& name, bool inAParty) {
    if (inAParty) return name + " was part of the party that defeated him.";
    return name + " single handedly defeated Asmodeus.";
}

// R3: a guild-log line for each survivor. R4: the vanished body has no rule effect.
void writeAsmodeusGuildLogs(Ctx& ctx, const Party& party, bool inAParty) {
    for (i32 i = 0; i < party.size; ++i) {
        const Character& ch = party.members[static_cast<u32>(i)];
        if (ch.isDead()) continue;  // survivors only
        Str line = asmodeusLogLine(ch.name, inAParty);
        ctx.say("guildlog.wrote", line, {}, {ch.name});
    }
}

// ---------------------------------------------------------------------------
// Teleporter trap, trap 7 (R5-R8)
// ---------------------------------------------------------------------------

// Teleporter destination validity: neither Rock (bit 19) nor Teleporter (bit 14) set.
bool isValidTeleportSquare(u32 wallWord) {
    return (wallWord & kWallMaskRock) == 0 && (wallWord & kWallMaskTeleporter) == 0;
}

// R6: up to 10000 tries; x then y, each rng.Next(30)+1 (1..30).
TeleporterDestination findTeleporterDestination(Ctx& ctx, const FloorState& floor) {
    TeleporterDestination dest{};
    for (i32 tries = 0; tries < kTeleportMaxTries; ++tries) {
        i32 x = ctx.rng.below(kTeleportAxisMax) + kTeleportAxisMin;
        i32 y = ctx.rng.below(kTeleportAxisMax) + kTeleportAxisMin;
        if (isValidTeleportSquare(floorWallWord(floor, x, y))) {
            dest.found = true;  // R7
            dest.x = x;
            dest.y = y;
            return dest;
        }
    }
    return dest;  // R8: not found, TeleportTo stays null
}

// R5: only the opener is moved. R7: teleport on success. R8: no move on failure.
TeleportTrapResult springTeleporterTrap(Ctx& ctx, Party& party, const FloorState& floor) {
    TeleportTrapResult result{};
    if (party.opener < 0 || party.opener >= party.size) return result;
    result.destination = findTeleporterDestination(ctx, floor);
    if (!result.destination.found) return result;  // R8
    Character& opener = party.members[static_cast<u32>(party.opener)];
    opener.location.x = result.destination.x;
    opener.location.y = result.destination.y;
    // UNKNOWN: opener.location.area is not recomputed on teleport (not stated in R7).
    result.teleported = true;
    ctx.say("trap.teleporter", opener.name + " is teleported!",
            {result.destination.x, result.destination.y}, {opener.name});
    return result;
}

// ---------------------------------------------------------------------------
// Chute falls (R9-R15)
// ---------------------------------------------------------------------------

// Pit-style roll [c4]: 5 + DungeonLevel/4 < Rnd*100 (formula from the source doc).
// UNKNOWN: engine body of the roll not in chunks; "/" read as float division.
bool pitStyleRoll(Ctx& ctx, i32 dungeonLevel) {
    double threshold = static_cast<double>(kPitRollBase) +
                       static_cast<double>(dungeonLevel) / kPitRollDepthDivisor;
    return threshold < ctx.rng.nextDouble() * kPitRollScale;
}

// Rules.HoversOverPit [c4]. UNKNOWN: body not in chunks. Assumed to be the pit-style
// roll, consistent with R10 (a levitating member who passes the roll floats).
bool hoversOverPit(Ctx& ctx, i32 dungeonLevel) {
    return pitStyleRoll(ctx, dungeonLevel);
}

// fall iff not (Levitating and Rules.HoversOverPit(rng, dungeonLevel)) [c4].
// VB And does not short-circuit, so the roll is drawn for every member; kept faithful.
bool chuteMemberFalls(Ctx& ctx, const Character& ch, i32 dungeonLevel) {
    bool levitating = ch.ailments.levitating;
    bool hovers = hoversOverPit(ctx, dungeonLevel);
    return !(levitating && hovers);
}

// Rules.ChuteDamage [c4]. UNKNOWN: body not in chunks. Returns 0 until supplied.
i32 chuteDamage(Ctx& ctx, i32 landedLevel, i32 raceSize) {
    (void)ctx;
    (void)landedLevel;
    (void)raceSize;
    return 0;
}

// R13: landing damage is 0 when Levitating, else Rules.ChuteDamage.
i32 chuteLandingDamage(Ctx& ctx, const Character& ch, i32 landedLevel, i32 raceSize) {
    if (ch.ailments.levitating) return 0;
    return chuteDamage(ctx, landedLevel, raceSize);
}

// Area of the square on the landed floor [c4].
i32 landedAreaId(const GameData& data, i32 landedLevel, i32 x, i32 y) {
    return floorAreaAt(data, landedLevel, x, y);
}

// Chute move [c4]: DungeonLevel = level; X, Y kept; Area = landed.AreaIdAt(x, y).
// CONFLICT: Original kept the area of the floor above. Port sets the lower floor's
// area "to match the source" (c4). The port behaviour is implemented here.
void moveFallenMember(Ctx& ctx, Character& ch, i32 landedLevel) {
    ch.location.dungeonLevel = landedLevel;
    ch.location.area = landedAreaId(ctx.data, landedLevel, ch.location.x, ch.location.y);
}

// R14: for anyone who fell, FALL then HIT is triggered.
void announceChuteFall(Ctx& ctx, const Character& ch, i32 damage) {
    ctx.say("chute.fall", ch.name + " falls through the chute!", {}, {ch.name});
    ctx.say("chute.hit", ch.name + " is hurt by the landing.", {damage}, {ch.name});
}

namespace {
void applyChuteFall(Ctx& ctx, Party& party, u32 idx, const FloorChute& chute,
                    i32 fromLevel, ChuteFallResult& out) {
    Character& ch = party.members[idx];
    // R12: the floor below by the chute record's depth.
    // UNKNOWN: FloorChute.depth field name assumed.
    i32 landed = fromLevel + chute.depth;
    moveFallenMember(ctx, ch, landed);                                   // R12
    i32 raceSize = raceSizeOf(ctx.data, ch.race);
    i32 dmg = chuteLandingDamage(ctx, ch, landed, raceSize);             // R13
    i32 hits = ch.vitals.curHits - dmg;
    ch.vitals.curHits = hits < 0 ? 0 : hits;  // UNKNOWN: death handling not in this part
    announceChuteFall(ctx, ch, dmg);                                     // R14
    out.fell[idx] = true;
    out.damage[idx] = dmg;
    out.fallCount += 1;
}
}  // namespace

// R9-R11: leader rolls first. If the leader falls and there are followers, the game
// asks whether to follow; "yes" sends every later member down with no roll.
// The party's own floor is not changed here (UNKNOWN: owner of party floor update).
ChuteFallResult walkOntoChute(Ctx& ctx, Party& party, const FloorChute& chute,
                              i32 fromLevel, bool followAnswer) {
    ChuteFallResult out{};
    bool leaderFell = false;
    for (i32 i = 0; i < party.size; ++i) {
        u32 idx = static_cast<u32>(i);
        bool falls = false;
        if (i == 0) {
            falls = chuteMemberFalls(ctx, party.members[idx], fromLevel);  // R9
            leaderFell = falls;
        } else if (leaderFell && followAnswer) {
            falls = true;  // R11: follows without a roll
        } else {
            falls = chuteMemberFalls(ctx, party.members[idx], fromLevel);  // R9
        }
        if (falls) applyChuteFall(ctx, party, idx, chute, fromLevel, out);
    }
    return out;
}

// R15: TAKE on a chute lands all members with no roll.
ChuteFallResult takeChuteLanding(Ctx& ctx, Party& party, const FloorChute& chute,
                                 i32 fromLevel) {
    ChuteFallResult out{};
    for (i32 i = 0; i < party.size; ++i) {
        applyChuteFall(ctx, party, static_cast<u32>(i), chute, fromLevel, out);
    }
    return out;
}

// Quirk [c4]: members left on the leader's square and floor disappear from view
// and are dead if hits are gone. Port note: they are recorded with the lower
// floor's area. UNKNOWN: which floor's area is meant; the party's floor is used.
void resolveChuteLeftBehind(Ctx& ctx, Party& party) {
    for (i32 i = 1; i < party.size; ++i) {
        Character& ch = party.members[static_cast<u32>(i)];
        bool sameSquare = ch.location.x == party.x && ch.location.y == party.y &&
                          ch.location.dungeonLevel == party.dungeonLevel;
        if (!sameSquare) continue;
        ch.visible = false;
        if (ch.vitals.curHits <= 0) ch.deathState = DeathState::Dead;
        ch.location.area = landedAreaId(ctx.data, party.dungeonLevel, ch.location.x, ch.location.y);
    }
}

// ---------------------------------------------------------------------------
// Hall of Records (R16-R19)
// ---------------------------------------------------------------------------

// Hall record check [c11].
// Deadliest: if power(value) <= power(stored) no change; otherwise stored = -1,
// which makes the following "value <= stored" test false, so the record changes.
// UNKNOWN: whether "stored = -1" is intended to force the comparison; the
// pseudocode is followed literally, which gives the same result here.
bool hallRecordBeaten(const GameData& data, RecordKind kind, const HallRecord& stored, i64 value) {
    if (kind == RecordKind::Deadliest) {  // R17
        i64 candidatePower = monsterPowerById(data, static_cast<i32>(value));
        i64 storedPower = monsterPowerById(data, static_cast<i32>(stored.value));
        if (candidatePower <= storedPower) return false;
        return true;  // stored = -1; value <= -1 is false for monster ids
    }
    return value > stored.value;  // if value <= stored.Value: no change
}

// R16, R18, R19: check, write, and announce unless the holder is the player.
// Deadliest is never announced.
bool checkAndLogHallRecord(Ctx& ctx, HallOfRecords& hall, RecordKind kind, i64 value,
                           const Str& holder, bool holderIsPlayer, i64 nowMs) {
    if (!hallRecordBeaten(ctx.data, kind, hallSlot(hall, kind), value)) return false;
    writeHallRecord(hall, kind, value, holder, holderIsPlayer, nowMs);  // R18
    if (!holderIsPlayer && kind != RecordKind::Deadliest) {             // R19
        announceHallRecord(ctx, kind, hallSlot(hall, kind));
    }
    return true;
}

// R18: write holder, value and date. The date is the explicit millisecond time.
// FIXED: only the value is stored; HallRecord has no holderIsPlayer or date field.
void writeHallRecord(HallOfRecords& hall, RecordKind kind, i64 value, const Str& holder,
                     bool holderIsPlayer, i64 nowMs) {
    (void)holder;
    (void)holderIsPlayer;
    (void)nowMs;
    hall.records[static_cast<u32>(kind)] = static_cast<i32>(value);
}

// Announcement text. FIXED: no date field, so the "Original RecordHolder" substitution
// is not applied; the stored holder is shown.
void announceHallRecord(Ctx& ctx, RecordKind kind, const HallRecord& rec) {
    Str holder = rec.holder;
    ctx.say("hall.record", holder + " set the " + recordKindName(kind) + " record.",
            {rec.value}, {holder});
}

// Starting records for a new game (table in the header). Missing file uses these.
// FIXED: records store values only; holder, holderIsPlayer and date are not kept.
HallOfRecords defaultHallOfRecords() {
    HallOfRecords hall{};
    for (i32 i = 0; i < kRecordKindCount; ++i) {
        const HallSeed& seed = kHallSeeds[i];
        hall.records[static_cast<u32>(seed.kind)] = static_cast<i32>(seed.value);
    }
    return hall;
}

// ---------------------------------------------------------------------------
// Monster power and skill indexes (R20-R24)
// ---------------------------------------------------------------------------

// Counts set bits among bits 0..bitCount-1.
i32 countLowBits(u32 flags, i32 bitCount) {
    if (bitCount < 0) bitCount = 0;
    if (bitCount > 32) bitCount = 32;
    i32 n = 0;
    for (i32 b = 0; b < bitCount; ++b) {
        if ((flags & (u32{1} << static_cast<u32>(b))) != 0) ++n;
    }
    return n;
}

// R20 [c11]: monster power for a live creature.
i64 monsterPower(const Creature& m) {
    return powerFromFields(m.att, m.str, m.hits, m.dex, m.guildLevel,
                           static_cast<u32>(m.specialBits),
                           m.specialPropertyFlags);
}

// R20 [c11]: monster power by monster id, through the static monster table.
// FIXED: MonsterDef has no str, dex or specialBits fields; those inputs are 0.
i64 monsterPowerById(const GameData& data, i32 monsterId) {
    const MonsterDef& def = monsterDefAt(data, monsterId);
    return powerFromFields(def.att, 0, def.hits, 0, def.guildLevel,
                           0u,
                           def.specialPropertyFlags);
}

// R21 [c11]: total = rating(4); for a in [2,1,3]: total = round_even(total + rating(a)*0.2);
// result = round_even(total / 160 * 100).
i32 fightingSkillIndex(const Arr<f32, 6>& ratings) {
    double total = static_cast<double>(ratings[4]);
    const i32 order[3] = {2, 1, 3};
    for (i32 a : order) {
        total = static_cast<double>(
            roundHalfEven(total + static_cast<double>(ratings[static_cast<u32>(a)]) * kFightingWeight));
    }
    return static_cast<i32>(roundHalfEven(total / kFightingDivisor * kSkillPercent));
}

// R22 [c11]: v = rating(0) * EffectiveDex; if v < 1: v = 1; result = round_even(v / 3500 * 100).
i32 thievingSkillIndex(f32 rating0, i32 effectiveDex) {
    double v = static_cast<double>(rating0) * static_cast<double>(effectiveDex);
    if (v < 1.0) v = 1.0;
    return static_cast<i32>(roundHalfEven(v / kThievingDivisor * kSkillPercent));
}

// R23 [c11]: possible = sum(spell.Level*10) + 255*count(spells).
// For each spell: first guild in table order with spell.Level > 0, SpellRate[Category] > 0
// and spell.Level*rate <= guildLevel/2 + 0.5 adds spell.Level*10, then
// have = round_even(have + guildLevel/2 + 0.5), and breaks out of the guild loop.
// UNKNOWN: the "level > 0" test is read as spell.Level > 0.
// UNKNOWN: SpellDef.level / .category and GuildDef.spellRate[] field names assumed.
i32 magicSkillIndex(const GameData& data, i32 guildLevel) {
    i64 possible = 0;
    for (const auto& s : data.spells) {
        possible += static_cast<i64>(s.level) * kMagicLevelScale + kMagicCapPerSpell;
    }
    double have = 0.0;
    double half = static_cast<double>(guildLevel) / 2.0 + 0.5;
    for (const auto& s : data.spells) {
        for (const auto& g : data.guilds) {
            double rate = static_cast<double>(g.spellRate[static_cast<u32>(s.category)]);
            if (s.level > 0 && rate > 0.0 && static_cast<double>(s.level) * rate <= half) {
                have += static_cast<double>(s.level) * kMagicLevelScale;
                have = static_cast<double>(roundHalfEven(have + half));
                break;
            }
        }
    }
    if (possible == 0) return 0;
    return static_cast<i32>(
        roundHalfEven(have / static_cast<double>(possible) * kSkillPercent));
}

// ---------------------------------------------------------------------------
// Pluralisation (R25-R26) [c7]
// ---------------------------------------------------------------------------

// R25: applied only when count > 1. R26: rules in table order, first match wins.
// UNKNOWN: "Replace last 2 with i" read literally: drop two characters, append "i".
Str pluraliseName(const Str& name, i32 count) {
    if (count <= 1) return name;

    const char* const unchanged[] = {"sae", "lex", "ish", "asu", "shi",
                                     "toa", "ngi", "ubi", "kul"};
    for (const char* s : unchanged) {
        if (endsWith(name, s)) return name;
    }
    if (endsWith(name, "yr") || endsWith(name, "ee")) return name;
    if (endsWith(name, "man")) return dropLast(name, 3) + "men";
    if (endsWith(name, "bus") || endsWith(name, "gus") || endsWith(name, "eet")) {
        return dropLast(name, 2) + "i";
    }
    if (endsWith(name, "ry") || endsWith(name, "py")) return dropLast(name, 1) + "ies";
    if (endsWith(name, "ch")) {
        return name == "Leech" ? name + "es" : name + "s";
    }
    if (endsWith(name, "sus")) return dropLast(name, 1) + "mi";
    if (endsWith(name, "ief") || endsWith(name, "arf") || endsWith(name, "Elf") ||
        endsWith(name, "olf")) {
        return dropLast(name, 1) + "ves";
    }
    if (endsWith(name, "mmy")) return dropLast(name, 1) + "ies";
    if (endsWith(name, "s")) return name;
    return name + "s";
}

}  // namespace mordor
