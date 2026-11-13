#include "test_support.h"

// Cases from B_0066 (dungeon chute, party-join, encounter and area-readout
// assertions). None of these can be asserted against the new headers as
// written, so each is recorded as a skip with its reason.

// SKIP B_0066#10: LookAround.Readout takes an on-screen character, facing and living/dead counts. roomReadout(party, roster, everywhere, listedDead) has no such inputs, and the header does not confirm it yields "(You are Dead)".
// SKIP B_0066#61: needs a scan of the shipped MDATA11.MDR chutes and their landing-floor area ids. The headers do not expose FloorDef's chute or area fields for that scan.
// SKIP B_0066#62: needs a real shipped chute record and multi-floor state to run Chute.MoveDown. walkChute/chuteBreakUp require data the test cannot build from the headers.
// SKIP B_0066#63: needs the same real chute drop as #62 to remove Fell from the party. Depends on chuteBreakUp/takeChute with shipped data.
// SKIP B_0066#64: Party.HereToJoin has no equivalent in the headers. Only joinList and askToJoin/leaderAliveForJoin exist, with different semantics.
// SKIP B_0066#65: Party.HereToJoin has no equivalent in the headers, and this case also depends on the chute drop from #62.
// SKIP B_0066#66: Party.WhyCannotJoin has no equivalent in the headers.
// SKIP B_0066#67: Party.HereToJoin has no equivalent in the headers. Also needs stale-record floor state the headers do not provide.
// SKIP B_0066#84: EncounterGenerator.Generate has no equivalent in the headers. Needs a full monster-group generation sweep over MDATA11 floors.
// SKIP B_0066#88: the arrival-room area id comes from the floor data in MDATA11.MDR. The headers do not expose the FloorDef area id for square (8,10), so the spawn mask cannot be reached.

