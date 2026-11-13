// dataformatsb.cpp: implements include/mordor/dataformatsb.h
// Source references are to SRD c-numbers, e.g. [c35].
// GuildLogEntry members used here: guild (guild index) and stamp (day serial; newest = largest stamp).
// city2a.h is unchanged.
#include "mordor/dataformatsb.h"
#include "mordor/city2a.h"  // FIXED: GuildLogEntry is declared in city2a.h

#include <cstddef>
#include <cstring>

namespace mordor {

namespace {

// [c13] table index i maps to file record i+4.
constexpr i64 kMonsterFileBase = 4;
// [c28] library record for monster index Ref is read at Ref + 6.
// UNKNOWN: relation between library Ref+6 and monster file record i+4 is not stated.
constexpr i64 kLibraryRefBase = 6;
// [c62] MDATA5 is the monster table; its record 0 is a duplicate and is skipped.
constexpr i32 kMonsterMdataFile = 5;
// [c28] Monster Id 307 is shared by the Morgul (278) and the Shadow Ninja (389).
constexpr i32 kAmbiguousMonsterId = 307;
// [c35] been_here = word & (1 << 23), gc1D50 = 23.
// UNKNOWN: whether automap indexes are 0- or 1-based is not stated.
constexpr u32 kAutomapBeenHereBit = 23;
// [c13] MDATA1 holds 7 header records before the tables start.
constexpr i64 kMdata1HeaderRecords = 7;

}  // namespace

// ---------------------------------------------------------------------------
// Option and flag values
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Record addressing
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Identification level
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Guild switching and checks
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// File names and duplicate check
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Little-endian field readers and writers  [c17][c35][c91]
// Integer and Word = 2 bytes, Long and Single = 4, Currency = 8.
// On a failed read the out value is set to 0 and false is returned.
// The port refuses instead of silently using 0 (see [c3]).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Automap
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Guild log (MDATA9.MDR)  [c10]
// Layout: header count u32 (written, ignored on read); then per entry:
//   guild index u32, day serial i64, byte length u32, text UTF-8.
// Reading stops at end of file. A truncated tail is dropped.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// MDATA1 header and table layout  [c13]
// Record size 260 bytes. Record 1 is the version. Records 2..7 are counts, in order:
// races, guilds, item types, item descriptions, monster classes, monster groups.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// MDATA file groups and names  [c96][c57]
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Mod settings  [c41]
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Store key and settings file  [c22][c34][c10]
// ---------------------------------------------------------------------------

}  // namespace mordor
