#include <cstring>

#include "test_support.h"
#include "mordor/dataformatsa.h"

using namespace mordor;

namespace {

template <class E>
i32 ev(E e) {
    return static_cast<i32>(e);
}

template <class V>
i32 sz(const V& v) {
    return static_cast<i32>(v.size());
}

IniEntry mkIni(const char* sec, const char* key, const char* val) {
    IniEntry e;
    e.section = sec;
    e.key = key;
    e.value = val;
    return e;
}

LiveMod mkLive(const char* id) {
    LiveMod m;
    m.id = id;
    return m;
}

Character mkChar(const char* name) {
    Character c;
    c.name = name;
    return c;
}

}  // namespace

// SRD: fixed layout and file-name constants (R2, R6, R4, R12, R14, tables).
TEST(dfa_record_layout_and_file_names) {
    CHECK_EQ(kCharRecordLen, 2900);
    CHECK_EQ(kCharRecordFirst, 3);
    CHECK_EQ(kGuildJoinedOffset, 1678);
    CHECK_EQ(kMdata1RecordSize, 260);
    CHECK_EQ(kPackSavedSlots, 41);
    CHECK(kNameForbiddenChar == '|');
    CHECK(std::strcmp(kGuildLogFileName, "MDATA9.MDR") == 0);
    CHECK(std::strcmp(kGuildLogBackupName, "MDATA9B.MDR") == 0);
    CHECK(std::strcmp(kFullFolder, "game\\full") == 0);
    CHECK(std::strcmp(kSharewareFolder, "game\\shareware") == 0);
    CHECK(std::strcmp(kSettingsFileName, "settings.ini") == 0);
    CHECK(std::strcmp(kLegacyIniFileName, "MORDOR.INI") == 0);
    CHECK(std::strcmp(kPreloadKey, "Character") == 0);
}

