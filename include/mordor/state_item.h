// state_item.h - one item instance (pack entry, chest, shop shelf, ground).
// Static item data (stats, spells, hands, swings, requirements, templates) lives in
// GameData (game_data.h) and is NOT repeated here. Dropped template fields: statMods,
// equipReqs, hands, swings, damageMod, grantsCritical, hasProtect, spellName, spellLevel,
// spellIsCasterLevel, classRestricted, alignMask, useLevel, template cursed/charges defaults.
// Dropped derived/legacy fields: shelvedBySelling (derived), known bitset (duplicate of idLevel),
// legacy identified bool (folded into idLevel at load).
#pragma once

#include "mordor/types.h"
#include "mordor/state_common.h"

namespace mordor {

// Guild-issued marker base: ailments 500..500+NumGuilds-1 marks a guild item. [R63][Terms]
constexpr i32 kGuildMarkerBase = 500;
// Crest item id base: crest id = 299 + guild index. [R4][R45]
constexpr i32 kCrestItemIdBase = 299;

// Alignment bits for alignFlags. Changed by Realign. [c52][c88]
constexpr u8 kAlignBitUnaligned = 0x01; // U
constexpr u8 kAlignBitGood = 0x02;      // G
constexpr u8 kAlignBitNeutral = 0x04;   // N
constexpr u8 kAlignBitEvil = 0x08;      // E

// Companion / item alignment tag. [R5][R29][R66][c22]
// CONFLICT: SRD encodings disagree (Good=1,Neutral=2,Evil=3 vs draw 0-3 vs tag encoding UNKNOWN).
//           Chosen: Unaligned=0 (sell-in note), Good=1, Neutral=2, Evil=3.
enum ItemAlign : i32 {
    Unaligned = 0,
    Good = 1,
    Neutral = 2,
    Evil = 3
};

// Identification level, ascending; it rises only. [c105][c66][R7][R50][R54]
// ALL is reachable only at the General Store. [c105]
// CONFLICT: save encodings differ (countdown id = 3 - Known [R29][c22]; 0=All [c32][c35];
//           port order Barely=0 [R7]). Chosen: ascending order below. Save mapping belongs to the save layer.
// UNKNOWN: Unknown as the starting state is per c105; Known=grade mapping [R49][R56][T-E] not reproduced.

// Chest trap kind. [c4][c64]
// UNKNOWN: numeric value of NotTrapped not given (0 chosen); other trap values UNKNOWN.

// Curse kind. [R44][R45][c63]
// CONFLICT: R44 says nonzero sets slotCursed and 2 means the item attaches; c63 says kinds 1 and 2,
//           kind 2 only from test, and a chest copy may store only a boolean.
// UNKNOWN: SRD gives no kind names; CurseKind1/CurseKind2 are placeholders.
enum CurseKind : i32 {
    CurseNone = 0,
    CurseKind1 = 1,
    CurseKind2 = 2
};

// Item kind. The first eight are consumables. [c21]
// UNKNOWN: Other is a placeholder for kinds not listed in the SRD.
enum ItemType : i32 {
    Potion = 0,
    Scroll = 1,
    Tome = 2,
    Dust = 3,
    Crystal = 4,
    Rod = 5,
    Sphere = 6,
    Cube = 7,
    Other = 8
};

struct Item {
    // Item table id. 0 = empty. Crest = 299 + guild index (kCrestItemIdBase). Quest fetch match key.
    // Ids beyond the table display as "#id". [R4][R45][R49][R59][c54]
    i32 id = 0;

    // Guild marker: 500..500+NumGuilds-1; guild index = ailments - 500. 0 = not a guild item. [R63][Terms]
    i32 ailments = 0;

    // Guild-issued item (crest or guild item). Cannot be given, dropped, sold or put in the vault. [R5][R17][R20][R27][R60]
    bool guildIssued = false;

    // Alignment flag bits (kAlignBit*). Changed by Realign.
    // UNKNOWN: [c59] store-add loops forever when no alignment bit matches; port behaviour not defined here.
    u8 alignFlags = 0; // [c52][c88]

    // true means the alignment check blocks use (aligned item check). [R1]
    bool aligned = false;

    // Companion/item alignment tag. See ItemAlign for the CONFLICT note. [R5][R29][R66][c22]
    ItemAlign alignment = Unaligned;

    // Charges left. 0 = out of charges. A cast needs > 0; spent once per completed cast; item removed at 0.
    // Refilled on town arrival / guild entry; Recharge refills. [R1][R19][R37][R39][R42][R51][R66][T12][c21][c44]
    // CONFLICT: [c52] says 0..32767 and overflow on combine exits the game; max otherwise UNKNOWN.
    i32 charges = 0;

    // Max charges for the guild. [R63][T12]
    // Crests 6 each (wiki) [c33][c104]. UNKNOWN: maxima for other guild items.
    i32 maxCharges = 0;

    // Chest trap kind. Not Trapped when no trap. [c4][c64]
    ChestTrap chestTrap = ChestTrap::NotTrapped;

    // Chest carries a teleporter trap; a lone opener is moved. [c46]
    // CONFLICT: teleporter is not in the ChestTrap enum. Kept as a separate flag.
    // UNKNOWN: rule number not given in the SRD census.
    bool chestTeleporter = false;

    // Chest curse kind. Nonzero sets slotCursed on chest attach. [R44][R45][c63]
    CurseKind cursed = CurseNone;

    // Whether the item may be destroyed by quicksand or destruction. [R42]
    // UNKNOWN: default not stated; false chosen.
    bool destructible = false;

    // Item is worn or wielded by its character. Cleared when stat requirements fail or on unequip. Refused for give. Default false. [R2][R10][R12][R13][R20][R22][R70][Table 5][c44][c54]
    bool equipped = false;

    // Monster id recorded for the find. -2 if the lead was taken for another or is invisible to the taker; otherwise the lead id. [R6][c12]
    // UNKNOWN: default not stated; 0 chosen (no monster). -2 is the sentinel; other meanings UNKNOWN.
    i32 findMonsterId = 0;

    // Identification level. Starts Unknown. Set to All on crest award (R59). Legacy identified=true maps to All at load. [R13][R59][c36][c66][c105]
    IdLevel idLevel = Unknown;

    // Lock level of a chest. Unlocks if <= spell opening level (effect - 29). [R60]
    i32 lockLevel = 0;

    // Item lost to water or quicksand. [R71]
    bool lost = false;

    // Item level (a wand's strength) and item spell level. Negative uses caster guild level;
    // 0 runs the level-0 gate; non-zero costs 0 SP and skips the unfit gate. [R3][R4][R5][R6][R18][R20][c1][c6][c43]
    i32 level = 0;

    // Square the item lies on.
    // UNKNOWN: Square type is not defined in this header; stored as a square index until one exists. [c20]
    i32 location = 0;

    // Pack slot curse flag. Set on chest attach, crest award, or equip-curse; not set on purchase.
    // Blocks unequip; cleared on unequip. Default false. [R21][R22][R44][c46][c63]
    bool slotCursed = false;

    // Spell id carried by the item. Gates on spellId > 0 (id 0 excluded in port). [R66]
    i32 spellId = 0;

    // 1-based index into the spell list (spell is all[spellIndex - 1]); must be valid to cast.
    // SRD name M56C4: one-based record position of the cast spell, 0 = none. [R36][c21][Quick reference]
    i32 spellIndex = 0;

    // Item kind. See ItemType. [c21]
    ItemType type = Other;

    // Guild item test: ailments marks guild items at 500 and above.
    bool isGuildItem() const { return ailments >= kGuildMarkerBase; }

    // Guild index of a guild item (valid only when isGuildItem() is true).
    i32 guildIndex() const { return ailments - kGuildMarkerBase; }
};

} // namespace mordor
