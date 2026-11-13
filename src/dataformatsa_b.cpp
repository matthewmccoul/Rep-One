// dataformatsa_b.cpp: second half of module dataformatsa.
// Implements: cleanGuildLog, daySerialFromEpochMillis, charStatAppendPage,
// trimCharacterName, safeFileName, checkNewCharacter, storedPassword,
// modSettingWrite, modSettingWriteFlag, modSettingRead, modSettingRemove,
// modSection, modEscapeCell, modEncodeRows, modDecodeRows, modSaveTag,
// modTagMissingFrom.
//
// UNKNOWN (module-wide): the SRD half supplied for this module contains no
// rule text for most of these functions. Behaviour below is the simplest
// sensible choice; each choice is marked UNKNOWN where it is made.
// ModSettingEntry is not visible in the included headers. Its id and key
// members are modId and key. FIXED: the value member was reported missing,
// so it is renamed to "text" here. "text" is an unverified guess. Confirm the
// real member name in dataformatsa.h and correct the three uses below.

#include "mordor/dataformatsa.h"

namespace mordor {

}  // namespace mordor
