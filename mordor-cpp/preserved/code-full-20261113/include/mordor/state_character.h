// state_character.h
// Plain data for one player character. Every field cites its SRD rule where one is given.
// Gold is Currency (i64, units of 1/10000 gold). No logic beyond tiny inline helpers.
#pragma once

#include "mordor/types.h"
#include "mordor/state_common.h"
#include "mordor/state_item.h"
#include "mordor/state_companion.h"

namespace mordor {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

inline constexpr i32 kStatCount = 6;            // R2, R3: Str, Int, Wis, Con, Cha, Dex
inline constexpr i32 kBankSlots = 40;           // R33: vault capacity
inline constexpr i32 kSpellBuffers = 10;        // R76: buffers 1..10
inline constexpr i32 kResistKinds = 11;         // R31: resistance kinds 0..10
inline constexpr i32 kCarryNone = -1;           // R37: carry index for nothing carried
inline constexpr i32 kAgeStartDays = 5840;      // R19, R20: 16 * 365 at creation

// ---------------------------------------------------------------------------
// Coded values
// ---------------------------------------------------------------------------

// Guild table order: 0 Nomad ... 11 Healer. R1, R4, T1.
enum class Guild : i32 {
    Nomad = 0,
    Warrior,
    Paladin,
    Ninja,
    Villain,
    Seeker,
    Thief,
    Scavenger,
    Mage,
    Sorcerer,
    Wizard,
    Healer
};

// Race index follows MDATA1 order. R1, R2, R15.
enum class Race : i32 {
    Human = 0,
    Elf,
    Giant,
    Gnome,
    Dwarf,
    Ogre,
    Morloch,
    Osiri,
    Troll
};  // UNKNOWN: default race not stated in the census; Human used as a placeholder.

// R5, R15, R19. Alignment must fall inside the race alignment mask.
enum class Align : i32 {
    Good = 0,
    Neutral = 1,
    Evil = 2
};  // UNKNOWN: default alignment not stated; Neutral used. CONFLICT: one line lists an extra Unaligned value; not kept.

// R8: 0 = Male, 1 = Female. Must be selected at creation.
enum class Sex : i32 {
    Male = 0,
    Female = 1
};  // UNKNOWN: default sex not stated; Male used.

// Table 1, Table 3, R3, R4: 0 = alive/ordinary death, 1..8 = named death states.

// Table 2: raise kinds. 23 = morgue.
enum class RaiseKind : i32 {
    None = 0,
    RaiseDead = 20,
    RestoreFlesh = 21,
    Resurrect = 22,
    Morgue = 23
};

// R29: combat setting. Buffer uses Character::combat.target as buffer index.
enum class CombatOrderKind : i32 {
    None = 0,
    Fight,
    Defend,
    Buffer
};  // CONFLICT: census also lists Spell and Item/UseItem variants; not kept here.

// R13, R32, R36: guild quest type.
enum class QuestKind : i32 {
    None = 0,
    Defeat = 1,
    Fetch = 2,
    Companion = 3   // UNKNOWN: companion quest engine status not stated.
};

// Quirks item quests: item quests never reach Done and clear at Make Level.
enum class QuestState : i32 {
    Active = 0,
    Done = 1,
    Cleared = 2
};

// Natural stat indices. R2, R3.
enum class StatIndex : i32 {
    Str = 0,
    Int = 1,
    Wis = 2,
    Con = 3,
    Cha = 4,
    Dex = 5
};  // CONFLICT: census lists index order as UNKNOWN in one line; order Str..Dex used (Con = 3).

// ---------------------------------------------------------------------------
// Nested groups
// ---------------------------------------------------------------------------

// Gold on hand and in bank. R34, R35, R47. Both are Currency.
struct Purse {
    Currency goldOnHand = 0;   // R34, R35: charges paid from here first. Default 0. CONFLICT: R25/R26 allow negative; R35 says >= 0.
    Currency goldInBank = 0;   // R34, R35, R41: paid after hand gold. Default 0. [c55] can go negative under charPay quirk.
};

// Hit points and spell points. R3, R5, R46.
struct Vitals {
    i32 curHits = 0;           // R3, R5, R19: 0..maxHits. Set to 0 on death (R3). CONFLICT: pit can drive it below zero (R53).
    i32 maxHits = 15;          // R5: 15 at creation. R18: floored at 15 on forfeit. Cap 999 (R19).
    i32 curSpellPoints = 0;    // R5, R6, R45: 0..maxSpellPoints. Refilled on request.
    i32 maxSpellPoints = 0;    // R46, R47: (INT + WIS) * 5 from natural stats.
};

// Ailments and status flags. R1, R3, R4, R9, R13, R14, R18, R28, R54.
struct Ailments {
    i32 poisonLevel = 0;       // R1, R9, R38: spawn level of poisoning monster. 0 = not poisoned.
    i32 diseaseLevel = 0;      // R1, R9, R38: 0 = not diseased. Cleared by CureDisease.
    i32 paralysisLevel = 0;    // R2, R13, R14: guild level of paralysing monster. 0 = not held (still strips stats per W7).
    i32 paralyzedRounds = 0;   // [c2]: countdown. UNKNOWN: units and range.
    i32 protectLevel = 0;      // R54: Protection counter. Cast sets Int(casterSL/2)+1; recompute sets Int(GuildLevel/2)+1.
    i32 ailmentSteps = 0;      // R3, R28: 0 cures poison and disease. Cleared on mid-round death.
    bool paralysed = false;    // R18, R26, R33: blocks equip, use, drop and info. Default false.
    bool rocked = false;       // [c84][c97]: held in Rock. Cleared by Retrieve Soul or Resurrect.
    bool levitating = false;   // R4, R42, R52: avoids pit, water and chute damage. Set by Levitate.
    bool invisible = false;    // R4, R44: set by Invisible cast or recompute.
    bool seeInvisible = false; // R4, R44, R46: can see invisible targets.
};

// Lost flags. R1, R9, R25.
struct LostState {
    bool position = false;     // R25, R29: lost position. Cleared by FoundYourself and town arrival.
    bool level = false;        // R25, R28: lost floor. Cleared by FoundYourself.
    bool facing = false;       // R25, R99: facing shown as "?" when set.
};

// Where the character is. R12, R19, R29.
struct Location {
    i32 x = 0;                 // Stored 0-based x, kept on rescue. R7, R10. CONFLICT: R10 and R19 also give 1-based 1..30.
    i32 y = 0;                 // Stored 0-based y. R7, R10. CONFLICT: see x.
    i32 dungeonLevel = 0;      // R2, R5, R9: 0 = town, 1..FloorCount in dungeon.
    i32 area = -1;             // R5, R19: -1 at creation and in town. 0 when dead in rock below ground (Terms).
    i32 facing = 0;            // 0..3 = N, E, S, W (port). R42. CONFLICT: original stores 1..4 (R2, R96).
};

// Attack and defence components. Terms (A/D), R10, R16, R17, R28, R36.
struct CombatRatings {
    i32 attack = 0;            // Terms (A/D): total attack score. R16, R21, T6 (capped across joined guilds).
    i32 defence = 0;           // Terms (A/D): total defence score. R17, R42 (x1.5 while Defend, disputed).
    f32 adAccum = 0.0f;        // Quirks A/D: accumulator; step is rounded to integer.
    i32 adShift = 0;           // R13, R14: stored Att/Def offset. -32 after gate-freeing. Default 0.
    i32 storedAdShift = 0;     // R13, R16, R17: saved shift held while stat part is held. Default 0.
    i32 statsAdNow = 0;        // W7, R13-R15: current stat part of Att/Def.
    i32 protection = 0;        // R28, R36: melee defence bonus from spell or gear. Protect sets Int(GuildLevel/2)+1.
    i32 swings = 0;            // R7, R10, R11: attack count from RecalcSwings. Never falls.
};

// Stats and gear modifiers. R1, R2, R3, R4, R6, R7, R25.
struct Abilities {
    Arr<i32, kStatCount> stats{};       // R2, R3, R46: natural stats in StatIndex order. Range race Min..Max, raise to race Max+5.
    Arr<i32, kStatCount> statMax{};     // R4, R5: per-stat maximum copied from race Max at creation.
    Arr<i32, kStatCount> statMods{};    // R6, R7: gear modifier total per stat. Default 0.
    i32 gearCon = 0;                    // R7: CON from gear. CONFLICT: port omits it in cure rolls.
    i32 gearWis = 0;                    // c19, CONFLICT-24: WIS from equipped items.
    i32 equipAbilityMask = 0;           // R6-R8: ability bits of equipped items (crit 0x10, backstab 0x20).
};

// Spell-granted resistances and protections. R31, R33c, R45.
struct Resistances {
    Arr<i32, kResistKinds> resist{};        // Percent 0..95 per ResistIndex. Resist spells set 95 (effects 50-60). Cleared on city gate.
                                             // CONFLICT: census also gives 6 kinds and 12 kinds; 11 kept per R31.
    Arr<i32, kResistKinds> protectionValues{};  // Eleven protection values. Cleared by Extinguisher.
    Vec<i32> protectionMasks{};             // Cleared by Extinguisher. UNKNOWN: count not stated.
    i32 poisonResist = 0;                   // c28, c64: poison test uses poisonResist/2. CONFLICT-6: direction disputed.
};

// Combat order and spell buffers. R5, R10, R12, R18, R29, R58.
struct CombatOrder {
    CombatOrderKind kind = CombatOrderKind::None;  // R29: default None.
    i32 target = 0;                                  // R29, R5: buffer index for Buffer; group index for attack.
    i32 aimedGroup = 0;                              // R15: aimed monster group; retargets when empty.
    i32 spellId = 0;                                 // R12, R14, R18, R20: queued spell id. 0 = none.
    Arr<i32, kSpellBuffers> spellBuffers{};          // R12, R58, R76: -1 empty; 0-39 pack slot; 40+i spell index. Default -1.
    bool defendOn = false;                           // R8: Defend active. Derived from kind; kept for readout. Default false.
};

// Quest state for one guild membership. R10, R13, R23, R32, R33, R36.
struct GuildQuest {
    QuestKind kind = QuestKind::None;        // R13, R32: None when no quest.
    QuestState state = QuestState::Active;   // Quirks item quests: default Active when assigned.
    i32 target = 0;                          // R10, R33: monster or item id. 0 when no quest.
    i32 level = 0;                           // R33: mLevel 1..10 or iLevel 1..7. 0 when no quest.
    i32 count = 0;                           // R23: kill or fetch progress. Only the killer counts.
    bool pinned = false;                     // R20.4, R56.3, R70: pinned with outstanding quest. UNKNOWN: source.
};

// One guild membership. R6, R9, R13.7, R15, R20, R22, R42, R80.
struct GuildMembership {
    bool joined = false;                     // R15, R17: holds this guild. Default false.
    i32 level = 0;                           // R6, R20: 0 = not joined; joined 1..999. CONFLICT: Nomad max 197 or 198.
    i64 experience = 0;                      // R13.7, R24: guild XP toward next level. Clamped to threshold-1 after a level.
    i32 att = 0;                             // [c61][c102]: guild attack bonus. Max across memberships is used. UNKNOWN default.
    i32 def = 0;                             // [c61][c102]: guild defence bonus. Max across memberships is used. UNKNOWN default.
    Arr<f32, 6> abilityRating{};             // T13, T18: Thieving..Perception ratings; Fighting stored divided by 10.
    Currency guildGold = 0;                  // R20: input to advance gold (guildGold/10). CONFLICT: also listed on Character.
    bool questDone = false;                  // R42, R54: Defeat quest completed.
    GuildQuest quest{};                      // R10, R13, R23: active quest for this guild.
};

// Carry, companions and inventory. R2, R37, R42, R103.
struct Inventory {
    Arr<Item, kPackSlots> pack{};            // R4, R56, R58: index 0 = Hands (never stored or swapped); 1..39 carried items.
                                             // CONFLICT: census also gives 41 slots and index 40 usable; 40 kept per target.
    Arr<Item, kBankSlots> bank{};            // R31-R33: vault of 40 slots; ItemId 0 = empty; holes allowed.
    Vec<Item> worn{};                        // R15, R24: equipped items. Cursed items block join. UNKNOWN: slot count.
    i32 freeHands = 2;                       // [c104][c107]: hands not holding wielded items, 0..2. Default 2.
    i32 carryIndex = kCarryNone;             // R37, R38, R42: -1 nothing; 0..3 companion slot; >=4 carried body.
                                             // UNKNOWN: carried body reference type not stated; only index kept.
    bool hidden = false;                     // R21, R30, R104: cannot be picked up or carried.
};

// ---------------------------------------------------------------------------
// The character
// ---------------------------------------------------------------------------

struct Character {
    // Identity. R1, R4, R8.
    Str name{};                              // R8: trimmed, non-empty, no '|'. Table 10.
    Str password{};                          // R8: empty stored as NoPassword sentinel.
    Str raisedBy{};                          // Table 10: name of the raiser. Empty default.
    Str carryingCharacterName{};             // R42: name of carried body's owner. "" when empty.
    Vec<Str> mods{};                         // R1, c3: mod tags (id@version) copied to Game.ModsWhenLoaded.
    Race race = Race::Human;                 // R1, R2, R5: selects race row. UNKNOWN default.
    Sex sex = Sex::Male;                     // R8.
    Align alignment = Align::Neutral;        // R2, R5, R19. UNKNOWN default.
    Guild activeGuild = Guild::Nomad;        // R2, R8, R14: guild used for ActiveGuildDef and checks. CONFLICT: also CurrentGuild/PrimaryGuild.

    // Progression. R5, R9, R10, R19, R23, R25.
    i32 level = 1;                           // R23, R64: character level, 1 or more. Newcomer protection at level 1 [c7].
    i64 experience = 0;                      // R5, R30: XP. 0 at creation.
    i64 xpRequired = 0;                      // R9, R10: XP to advance, scaled by race multiplier. UNKNOWN units/formula.
    i32 ageDays = 0;                         // R19, R20, R25, R82: age in days. Creation sets kAgeStartDays (5840). CONFLICT: census also gives years and weeks.
    i32 topGuildLevel = 0;                   // R13.2, R10, R35: highest membership level. Derived; stored for reads.
    i32 spellLevel = 0;                      // R6, R7: caster spell level SL, 0..255. UNKNOWN default.
    i32 perception = 0;                      // R37: notice roll passes when rng.below(100)+1 <= perception. Range 0..200.
    i32 pointsPool = 0;                      // R3, R4, R8: unspent creation points. UNKNOWN: default 30 + bonus points.

    // Guilds. R1, R2, R4, R17, R22, T1.
    Arr<GuildMembership, kGuildCount> guilds{};  // One slot per guild in table order. Default all unjoined.

    // Body and survival.
    Vitals vitals{};                         // R3, R5, R18, R46.
    Purse wallet{};                          // R34, R35, R47.
    Ailments ailments{};                     // R1, R4, R9, R28, R54.
    LostState lost{};                        // R1, R9, R25.
    Location location{};                     // R2, R10, R12, R19, R29.
    i32 daysAged = 0;                        // R30: days aged; restock on arrival only when leader's value > 0.
    i32 raiseChance = 0;                     // R11, R12: pending raise chance percent 0..98. >0 means pending.
    RaiseKind raiseKind = RaiseKind::None;   // Table 2, R11, R12: 0 when no raise.
    bool pendingRaise = false;               // R8: dead joiner admitted when true.
    DeathState deathState = DeathState::Alive;  // Table 1, R3, R4, R5, R12: death state. Alive default.
    i32 deaths = 0;                          // R3, R5, R46: per-character death counter. Default 0.
    bool visible = true;                     // [c4]: false for members left behind after sub0908. Default true.
    bool saved = false;                      // R57: Locate lists only saved characters. Default false.
    bool noPartySound = false;               // R32: No Party Sound option; gates death cry.
    bool misdirected = false;                // R45: cancelled when owner is drawn.
    bool forLife = false;                    // R41: protected from Fear release.
    bool castThisRound = false;              // R43: one-cast-per-round gate for owner's companions.
    bool chuteFloats = false;                // [c45][c66]: per-member chute float roll result. UNKNOWN for levels other than 8.

    // Combat. R5, R10, R20, R29, R41.
    CombatRatings combat{};                  // Terms (A/D), R10, R16, R17, R28.
    CombatOrder order{};                     // R5, R12, R18, R29.
    Abilities abilities{};                   // R1, R2, R4, R6, R7, R25.
    Resistances resistances{};               // R31, R33c, R45.
    i64 weaponBlows = 0;                     // R32: swings counted as weapon blows.
    i32 kills = 0;                           // R31, R46, R49: kill count. A charm adds one kill (port).
    i64 damageRecorded = 0;                  // R31: hits recorded as damage.
    Vec<i32> monsterKills{};                 // R46: monster ids recorded killed by NoteMonster.
    i32 itemOrderDrain = 0;                  // R43: item-order drain state. UNKNOWN: meaning not stated.
    i32 chestCountdown = 0;                  // R22: rounds before opening a chest. Observed 2..10 (T6).
    bool openingChest = false;               // R30, R35: adds +0.75 to damage modifier while opening.
    bool castStateSet = false;               // R18: flag tested before cast check; false when spell list opened.

    // Spells. R23, R40, R76.
    Vec<i32> knownSpellIds{};                // R1, R23: spell ids the character knows.
    Vec<i32> activeEffectIds{};              // R40, [c4]: active spell effect ids. UNKNOWN: SpellEffect record layout not stated.

    // Guild quest status that is not per membership.
    i32 itemQuestsDone = 0;                  // R45: fetch quests turned in, >= 0. Scope assumed per character.

    // Legacy and clock fields.
    Vec<Vec<bool>> explored{};               // R9: legacy per-character explored map read by AbsorbLegacy.
    i64 playSeconds = 0;                     // R7: accumulated play time in 1/10000 s. Default 0.
    i64 playStart = 0;                       // R7, R30: last tick time in seconds of day, 0..86399.

    // -----------------------------------------------------------------------
    // Tiny inline helpers only. No rule logic here.
    // -----------------------------------------------------------------------
    bool isDead() const {
        return deathState != DeathState::Alive;   // CONFLICT: 0 is both alive and ordinary death; see DeathState.
    }

    bool isLost() const {
        return lost.position || lost.level;       // R9, R25: derived.
    }

    Currency purse() const {
        return wallet.goldOnHand + wallet.goldInBank;   // Terms, Quick Ref: Gold + BankGold.
    }

    i32 activeLevel() const {
        return guilds[static_cast<u32>(activeGuild)].level;   // R55, R72: active guild level.
    }
};

}  // namespace mordor
