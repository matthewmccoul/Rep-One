#include "test_support.h"

// SKIP B_0061#156: Rules.QuestTier has no equivalent function in the provided headers.
// SKIP B_0061#157: Rules.QuestItemBand has no equivalent function in the provided headers.
// SKIP B_0061#158: Rules.QuestItemBand has no equivalent function in the provided headers.
// SKIP B_0061#159: Rules.RollQuest has no equivalent function in the provided headers (guild quest rolling is not declared).
// SKIP B_0063#76: The automap sheet flag-drawing loop (Floor.STUD) has no equivalent function in the provided headers.
// SKIP B_0063#77: The automap sheet flag-drawing loop (Floor.RESCUE) has no equivalent function in the provided headers.
// SKIP B_0063#78: Perception.Noticeable has no equivalent function in the provided headers; rollSquarePerception does not expose per-square-kind results for FACE_* squares.
// SKIP B_0063#79: Perception.Noticeable has no equivalent function in the provided headers; the count of noticeable square kinds is not exposed.
// SKIP B_0063#80: perceiveUnderfoot is the closest candidate, but its header does not document automap writes for teleporter squares, and the "already mapped" setup depends on automap/explored encoding the headers do not define.
// SKIP B_0063#81: perceiveUnderfoot is the closest candidate, but its header does not document how a lost leader suppresses automap writes for teleporter squares, so the expected outcome cannot be reliably set up.
