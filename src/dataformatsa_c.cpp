// dataformatsa_c.cpp
// Implements: planInstallerUnpack, encodeStoreShelf, loadStoreShelf, storeKey,
// applyLegacyIdentified, goldToSaveFields, goldFromSaveFields,
// warnStoredGuildStatsDiffer, dataFileClass, mayDeleteDataFile,
// keepsTableRecordZero, recordOffset, charRecordOffset, automapIndex, automapBeenHere.
//
// NOTE: The SRD half for the installer, store JSON, legacy identified flag and
// guild-stats warning was not supplied to this ant. Those functions are marked
// UNKNOWN and use the simplest sensible behaviour. Field names used on
// Shelf, GuildAttDefResult and ModSettingEntry are assumed and must be checked
// against items3a.h, guilds3.h and dataformatsa.h.

#include <string>

#include "mordor/dataformatsa.h"

namespace mordor {

// ---------------------------------------------------------------------------
// Installer (SRD c57 gives only the installer's layout, not its file format).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Store JSON (store.json). Format not given in the SRD half supplied.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Legacy identified flag.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Gold save fields. Currency is i64 in 1/10000 gold [c17][c35].
// Save form: whole gold plus an exact Currency value when the balance has a
// fractional part.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Guild stats warning.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Data file classification and deletion.
// Vital and non-vital lists come from dataformatsa.h. Names are compared exactly
// as written (uppercase, per the lists). UNKNOWN: DOS names are case-insensitive;
// callers are assumed to pass upper-case names.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Table record 0 handling [c20p1 table].
// MDATA2 (spells) and MDATA3 (items): record 0 is a real record, not skipped.
// MDATA5 (monsters): record 0 is the duplicate "GOBLIE", skipped.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Record offsets. Records are 0-based: offset = recnum * recordLength, because
// record 0 is a real record [c20p1 table]. UNKNOWN: whether the original's
// 1-based VB Random convention applies here.
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// Automap. Level stride 900, row stride 30, index offset 2, beenHere bit 23
// (dataformatsa.h). UNKNOWN: x/y ordering within the level block is assumed
// to be row-major (y rows of 30, x within row).
// ---------------------------------------------------------------------------

}  // namespace mordor
