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

UnpackPlan planInstallerUnpack(const Ctx& ctx, const Vec<u8>& installerFile) {
    (void)ctx;
    UnpackPlan plan;
    // UNKNOWN: the installer signature and edition-detection rule are not in the
    // SRD half supplied. Until they are known, no file is treated as an installer.
    // Preferred default: outcome NotAnInstaller, edition Full, folder empty.
    if (installerFile.empty()) {
        return plan;
    }
    plan.outcome = UnpackOutcome::NotAnInstaller;
    plan.edition = UnpackEdition::Full;
    plan.folder = Str{};
    return plan;
}

// ---------------------------------------------------------------------------
// Store JSON (store.json). Format not given in the SRD half supplied.
// ---------------------------------------------------------------------------

Str encodeStoreShelf(const Ctx& ctx, const Shelf& shelf) {
    (void)ctx;
    (void)shelf;
    // UNKNOWN: store.json layout for Shelf not in the SRD half supplied.
    // Placeholder: returns an empty document. Must be replaced before use.
    return Str{};
}

Shelf loadStoreShelf(Ctx& ctx, const Str& storeJson, bool fileMissing) {
    (void)ctx;
    (void)storeJson;
    Shelf shelf{};
    // UNKNOWN: store.json parsing not in the SRD half supplied.
    // Missing file gives an empty shelf. Present file currently also gives an
    // empty shelf (placeholder).
    if (fileMissing) {
        return shelf;
    }
    return shelf;
}

Str storeKey(const Ctx& ctx, i32 itemId, ItemAlign alignment) {
    (void)ctx;
    // Item record key form is "i:id" [c28]. Alignment suffix is an assumption.
    // UNKNOWN: whether aligned copies of an item use a separate store key.
    Str key = "i:" + std::to_string(itemId);
    if (alignment != Unaligned) {
        key += ":" + std::to_string(static_cast<i32>(alignment));
    }
    return key;
}

// ---------------------------------------------------------------------------
// Legacy identified flag.
// ---------------------------------------------------------------------------

void applyLegacyIdentified(Ctx& ctx, Item& item, bool hasKnownField, bool identifiedFlag) {
    (void)ctx;
    // UNKNOWN: SRD rule not supplied. Simplest sensible rule: a modern
    // known-level field wins; otherwise the legacy boolean maps to All / Unknown.
    if (hasKnownField) {
        return;
    }
    item.idLevel = identifiedFlag ? All : Unknown;
}

// ---------------------------------------------------------------------------
// Gold save fields. Currency is i64 in 1/10000 gold [c17][c35].
// Save form: whole gold plus an exact Currency value when the balance has a
// fractional part.
// ---------------------------------------------------------------------------

ExactGold goldToSaveFields(Currency balance) {
    ExactGold fields;
    fields.whole = currencyWhole(balance);
    fields.exact = balance;
    // UNKNOWN: the exact field is written only when the balance is not a whole
    // number of gold. The SRD half supplied does not confirm this.
    fields.hasExact = (balance % kGoldExactScale) != 0;
    return fields;
}

Currency goldFromSaveFields(const ExactGold& fields) {
    // R14.4: an exact value whose whole part does not match the integer is ignored.
    if (fields.hasExact) {
        i64 exact = static_cast<i64>(fields.exact);
        // Whole part in 1/10000 units, floored (VB Int semantics).
        i64 wholePart = exact / 10000;
        if (exact < 0 && exact % 10000 != 0) {
            wholePart -= 1;
        }
        if (wholePart == static_cast<i64>(fields.whole)) {
            return exact;
        }
    }
    return currencyFromWhole(fields.whole);
}

// ---------------------------------------------------------------------------
// Guild stats warning.
// ---------------------------------------------------------------------------

void warnStoredGuildStatsDiffer(Ctx& ctx, i32 guildIndex, i32 storedAtt, i32 storedDef,
                                const GuildAttDefResult& recomputed) {
    // UNKNOWN: field names att/def on GuildAttDefResult are assumed (guilds3.h).
    // Event payload nums are assumed to be a Vec of i64.
    if (storedAtt == recomputed.att && storedDef == recomputed.def) {
        return;
    }
    ctx.say("guild.stats_differ",
            "Stored guild attack and defence differ from the recomputed values.",
            {static_cast<i64>(guildIndex), static_cast<i64>(storedAtt),
             static_cast<i64>(storedDef), static_cast<i64>(recomputed.att),
             static_cast<i64>(recomputed.def)},
            {});
}

// ---------------------------------------------------------------------------
// Data file classification and deletion.
// Vital and non-vital lists come from dataformatsa.h. Names are compared exactly
// as written (uppercase, per the lists). UNKNOWN: DOS names are case-insensitive;
// callers are assumed to pass upper-case names.
// ---------------------------------------------------------------------------

DataFileClass dataFileClass(const Ctx& ctx, const Str& fileName) {
    (void)ctx;
    constexpr i32 vitalCount =
        static_cast<i32>(sizeof(kVitalDataFiles) / sizeof(kVitalDataFiles[0]));
    constexpr i32 nonVitalCount =
        static_cast<i32>(sizeof(kNonVitalDataFiles) / sizeof(kNonVitalDataFiles[0]));
    for (i32 i = 0; i < vitalCount; ++i) {
        if (fileName == kVitalDataFiles[i]) {
            return DataFileClass::Vital;
        }
    }
    for (i32 i = 0; i < nonVitalCount; ++i) {
        if (fileName == kNonVitalDataFiles[i]) {
            return DataFileClass::NonVital;
        }
    }
    return DataFileClass::Unlisted;
}

bool mayDeleteDataFile(const Ctx& ctx, const Str& fileName) {
    // UNKNOWN: SRD rule for deletion not supplied. Simplest sensible rule:
    // only listed non-vital files may be deleted. Vital and unlisted files are kept.
    return dataFileClass(ctx, fileName) == DataFileClass::NonVital;
}

// ---------------------------------------------------------------------------
// Table record 0 handling [c20p1 table].
// MDATA2 (spells) and MDATA3 (items): record 0 is a real record, not skipped.
// MDATA5 (monsters): record 0 is the duplicate "GOBLIE", skipped.
// ---------------------------------------------------------------------------

bool keepsTableRecordZero(const Ctx& ctx, i32 mdataNumber) {
    (void)ctx;
    if (mdataNumber == 5) {
        return false;  // MDATA5 record 0 is a duplicate of Outcast Goblin: skip.
    }
    if (mdataNumber == 2 || mdataNumber == 3) {
        return true;   // Real records: do not skip.
    }
    // UNKNOWN: record 0 handling for other MDATA tables is not in the SRD half
    // supplied. Preferred default: do not skip.
    return true;
}

// ---------------------------------------------------------------------------
// Record offsets. Records are 0-based: offset = recnum * recordLength, because
// record 0 is a real record [c20p1 table]. UNKNOWN: whether the original's
// 1-based VB Random convention applies here.
// ---------------------------------------------------------------------------


i64 charRecordOffset(i32 n) {
    // Character records start at record kCharRecordFirst (3) with length
    // kCharRecordLen (2900) [dataformatsa.h]. UNKNOWN: whether n is a 0-based
    // slot index. Assumed 0-based.
    return recordOffset(kCharRecordLen, static_cast<i64>(kCharRecordFirst) + static_cast<i64>(n));
}

// ---------------------------------------------------------------------------
// Automap. Level stride 900, row stride 30, index offset 2, beenHere bit 23
// (dataformatsa.h). UNKNOWN: x/y ordering within the level block is assumed
// to be row-major (y rows of 30, x within row).
// ---------------------------------------------------------------------------

i32 automapIndex(i32 level, i32 x, i32 y) {
    return kAutomapIndexOffset + level * kAutomapLevelStride + y * kAutomapRowStride + x;
}


}  // namespace mordor
