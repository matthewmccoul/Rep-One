#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/rng.h"

using namespace mordor;

// SKIP B_0056#10: EncounterGenerator.InRoom is not declared in our API, and there is no draw-counting IRng wrapper for a stored-and-reloaded room with an encounter.
// SKIP B_0066#140: EncounterLoopsProbe.Analyse is a probe over the MDATA5.MDR file. We have no MDATA5 loader or monster-table check declared.
// SKIP B_0066#141: EncounterLoopsProbe.Analyse builds encounter state tables from MDATA5.MDR. Those tables and their like-types loop analysis are not declared in our API.
// SKIP B_0066#142: EncounterLoopsProbe.Analyse builds encounter state tables from MDATA5.MDR. Those tables and their restart-state analysis are not declared in our API.
// SKIP B_0062#13: The count runs over all 15 floors' squares, areas and spawn masks. The floor/area/spawn-mask schema in GameData is not in the declarations shown. Also, the shipped data is the shareware set (fewer than 15 floors), so this needs fullGameOnly().
// SKIP B_0062#14: Same as B_0062#13. It counts Stud squares on all 15 floors using the spawn mask and the encounter floor, and needs the floor/area schema plus fullGameOnly().
// SKIP B_0062#15: Same as B_0062#13. It counts non-Stud squares for Shadow Ninja over all 15 floors, needing the floor/area schema plus fullGameOnly().
// SKIP B_0062#16: Same as B_0062#13. It counts Stud squares for Shadow Ninja over all 15 floors, needing the floor/area schema plus fullGameOnly().
