# DelveDeeperContent plugin: first local Unreal test

Date: 2026-11-12. Engine: UE 5.8.3 (Win64, VS 2022 Build Tools 17.14). Source: `DelveDeeper_T001_content_and_code.zip` from the cloud session "Google Drive START_HERE file", folder `code/unreal/DelveDeeperContent`.

## Result
- Compiles and links in the editor target with **0 warnings** after 4 small fixes (below).
- Loads in the editor. All 17 smoke checks in `Tests/smoke_test.py` pass (`Tests/smoke_result.txt`).

## Fixes needed (feed these back to the cloud session)
1. **`DelveDeeperContent.Build.cs`**: delete `CppStandard = CppStandardVersion.Cpp17;`. UE 5.8 rejects C++17 ("no longer allowed"). The core compiles fine as C++20.
2. **`DelveDeeperContent.uplugin`**: delete `"EngineVersion": "5.4.0"`. With it, UE 5.8 skips loading the plugin ("Skipping load of 'DelveDeeperContent'").
3. **`DDFloorMapActor.cpp`** lines ~129 and ~192: local `TArray<FName> Tags` shadows `AActor::Tags` (error C4458, warnings are errors). Renamed to `PieceTags`.
4. **`DDPuzzleComponent.h` / `DDQuestComponent.h`**: `TUniquePtr<FDDPuzzleImpl>` / `TUniquePtr<FDDQuestImpl>` fail with C4150 (UHT-generated code deletes the incomplete type, even though the destructor is out of line). Fix: an out-of-line deleter, `struct FDDPuzzleImplDeleter { void operator()(FDDPuzzleImpl*) const; };`, defined in the .cpp as `delete Ptr;`, the member as `TUniquePtr<FDDPuzzleImpl, FDDPuzzleImplDeleter>`, and `Impl = MoveTemp(NewImpl)` changed to `Impl.Reset(NewImpl.Release())`. Same for Quest.

5. **Build Floor gave no visible feedback** (problems only went to a BlueprintImplementableEvent; the pieces could spawn off-screen). Added `ReportProblem`/`ReportMessage` in `ADDFloorMapActor`: logs to `LogDDFloor`, shows an editor notification with the counts (or the problem), and moves the editor viewport to the new pieces. Build.cs adds `UnrealEd`, `Slate`, `SlateCore` for editor builds only. Confirmed in the editor: the floor is visible after Build Floor.

Each edit is marked `LOCAL TEST FIX` where a comment fits. The core copies (`Public/Core`, `Private/Core`) needed no changes.

## What the smoke test covered (editor, headless, -nullrhi)
- `UDDMapLibrary`: ValidateFloor (no problems), LoadFloor (17 nodes, 15 edges), FindReachable (17 from town_entrance; 16 with e08 blocked, storeroom excluded).
- `ADDFloorMapActor`: BuildFloor spawned 44 tagged pieces (17 node, 15 door, 12 slot); ClearFloor removed them.
- `UDDPuzzleComponent`: wrong control resets; pick → cart → furnace solves; door opens via "puzzle"; ClaimXp once only; save/load state round trip; pick_lock bypass opens with no XP.
- `UDDQuestComponent`: Offer → Accept → Observe ore_lift → Report reaches `completed` with the authored lines; save/restore round trip; StageFromScore(50) = 0.

## Not tested yet
- Walking the floor in Play In Editor (collision), Blueprint node pins, events (`OnUsed`, `OnLine`, etc.) bound in Blueprints, packaging and data staging.
