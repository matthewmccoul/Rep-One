# DelveDeeperContent: Unreal plugin (PROPOSAL)

Version 0.1 | Task T001-s014-software | Author role: development-software-engineer | Status: needs human review, **NOT RUN - Unreal missing**
Inputs read: brief `work/T001/briefs/s013-software-engineer-unreal.md`, GDD parts (AI Architecture, Technical Strategy, Coordination), core headers, QA spec UNR-06 to UNR-12.
Evidence: EXECUTED only for `sync_core_into_plugin.py` and `check_unreal_wrapper.py` (logs below). Everything else is STATIC (written, never compiled).

**Nothing here has been compiled, loaded or run in Unreal.** The C++ architecture (plain core plus thin Unreal wrappers) is a PROPOSAL until the user approves it.

## Engine version targeted
UE 5.4 (`EngineVersion` 5.4.0 in the `.uplugin`). WebSearch (standard) found only forum and vendor posts, no official page for the `.uplugin` fields or the `ModuleRules` options; so the Build.cs and `.uplugin` below are from memory of the 5.x conventions plus those posts. Check them against the 5.4 docs/source.

## Install
1. Copy the folder `DelveDeeperContent/` into `<YourProject>/Plugins/` (a C++ project is needed: the plugin has source).
2. Regenerate project files, build the project's editor target, open the editor, enable the plugin if asked.
3. Packaged builds: data files are plain text, not assets. Add `DelveDeeperContent/Content/Data` under Project Settings > Packaging > "Additional Non-Asset Directories to Copy" (or `DirectoriesToAlwaysStageAsNonUFS`). Not tested.
4. Drop `DDFloorMapActor` into a level, set nothing, click **Build Floor** in Details (it is `CallInEditor`).

## Layout
```
DelveDeeperContent/
  DelveDeeperContent.uplugin
  Content/Data/            floor1_map.json, storeroom_puzzle.json, figure_ore_lift_quest.json, figure_ore_lift_strings.csv  (generated copies)
  Source/DelveDeeperContent/
    DelveDeeperContent.Build.cs     C++17, exceptions off, RTTI off, unity off, include path Public/Core
    Public/  DDText.h DDTypes.h DDPuzzleComponent.h DDQuestComponent.h DDMapLibrary.h DDFloorMapActor.h
    Private/ DDText.cpp DelveDeeperContent.cpp DDPuzzleComponent.cpp DDQuestComponent.cpp DDMapLibrary.cpp DDFloorMapActor.cpp
    Public/Core/dd/*.h, Private/Core/*.cpp     generated core copies (banner on line 1, do not edit)
```

## Classes
| class | kind | exposes to Blueprints |
|---|---|---|
| `UDDPuzzleComponent` | UActorComponent | `LoadPuzzle`, `UseControl`, `OpenOther` (bypass), `ClaimXp`, `ResetProgress`, `GetStateName/GetSummary/GetProgress/IsDoorOpen/GetOpenedVia`, `SaveStateText`/`LoadStateText`; events `OnUsed(FDDUseOutcome)`, `OnDoorOpened(OpenedVia)`. Wraps `dd::puzzle::Puzzle`. |
| `UDDQuestComponent` | UActorComponent | `LoadQuest`, `SetRewardConfig`, `Offer`, `Accept`, `Decline`, `Observe` (core `arrive`), `Report` (caller supplies `Roll`), `GetRecord`, `StageFromScore`, `SaveStateText`/`RestoreStateText`; events `OnLine(FDDQuestLine)` (the line to show) and `OnStep`. Wraps `dd::quest::Engine`. |
| `UDDMapLibrary` | UBlueprintFunctionLibrary | `ReadDataFile`, `LoadFloor` (nodes and edges as structs), `ValidateFloor`, `FindReachable`. Wraps `dd::map`. |
| `ADDFloorMapActor` | AActor | `BuildFloor`/`ClearFloor` (CallInEditor), `MapFile`, `CellSize`, events `OnFloorProblem`, `OnFloorBuilt`. Spawns `AStaticMeshActor` pieces from the engine's basic shapes: a flat slab per place, a sphere per site inside a room, a strip per door (colour from a hash of the door text), small spheres for encounter/trap/loot slots. Pieces are tagged `DDId:<id>` and `DDType:`/`DDDoor:`/`DDSlot:` and attached to the actor. Slabs are 20 uu thick and have default collision: intended to be walkable; unverified. |

Helpers (not classes): `DDText.h/.cpp` (FString/std::string conversion, `LoadDataFile` from the plugin's `Content/Data/` through `IPluginManager`), `DDTypes.h` (USTRUCT/UENUM value types).
Wrappers hold no rules and no content ids; the component data-file names are editable defaults.
Quest `Roll`: the wrapper passes it to the core unchanged; the core defines the valid range (0 <= x < 1), so in Blueprints use "Random Float".
Not in the wrappers: save-game integration (the components give text; who stores it and when is for the save system), a selection UI for Use, hooking `Observe` to player movement.

## Data and core copies
Run `python3 -I work/T001/code/tools/sync_core_into_plugin.py` (optional `--log FILE`). It copies the headers of `common`, `map`, `puzzle`, `quest` to `Public/Core/dd/`, their sources to `Private/Core/` (each with a one-line "generated, do not edit" banner, otherwise identical), and the four data files to `Content/Data/` (identical). It deletes the old generated Core files first, prints each copy, and exits 1 with nothing written if a source is missing. Re-run it after any change to the core or data.

## What was run (EXECUTED)
| command | exit | log |
|---|---|---|
| `python3 -I work/T001/code/tools/sync_core_into_plugin.py --log ...` | 0, 5 headers, 5 sources, 4 data files | `work/T001/code/logs/sync_core_into_plugin.log` |
| `python3 -I work/T001/code/tools/check_unreal_wrapper.py work/T001/code/unreal --log ...` | 0, 4 classes, 0 violations, core calls > 0 for each | `work/T001/code/logs/check_unreal_wrapper.log` |
| same checker on scratch copies (flagged `Control == "ctl_cart"`; wrapper with core calls removed; empty dir) | 1, 1, 1 as UNR-09/UNR-10 require | not logged to file (shell redirect needed approval); output seen in the run |
| `check_no_unreal.py` (core) and `--cmake` | 0, 0 | rerun on this tree, not logged |

The checker is heuristic (regex, not a parser): it counts `dd::` calls and member calls whose names appear in the core headers, per class from its `.cpp`. It does not prove the wrappers are thin; QA/human review does that.

## NOT RUN - Unreal missing
- Compiling the plugin, UHT (header tool) processing, linking, loading in the editor (UNR-11).
- Blueprint exposure, component behaviour, events firing, `BuildFloor` output, walking the floor, packaging, data staging (UNR-12).
- The generated core copies compiled inside Unreal's toolchain (MSVC/Clang flags, warnings-as-errors).
- Nothing is tested by me; QA's review decides.

## Check these first when compiling (assumptions about Unreal APIs)
1. `.uplugin` field set and `Installed:false`; `Build.cs` options `CppStandard = CppStandardVersion.Cpp17`, `bEnableExceptions`, `bUseRTTI`, `bUseUnity` (names from memory; the 5.4 default standard is C++20).
2. Core `.cpp` files may trip Unreal warning-as-error settings (shadowed variables, unused, undefined identifiers) and Windows `min`/`max` macros; the core is standard C++ built with other flags here.
3. Does the engine with exceptions off accept `std::string`/`std::map`/`std::unique_ptr` in the module (it should) and the MSVC STL's `_HAS_EXCEPTIONS` setting.
4. `TUniquePtr<Incomplete>` as a member of a UCLASS with the destructor defined in the .cpp (UHT/CDO generation).
5. `FTCHARToUTF8(const TCHAR*, int32)`, `FUTF8ToTCHAR(const char*, int32)`, `FString(int32, const TCHAR*)` in `DDText.h`.
6. `IPluginManager::FindPlugin("DelveDeeperContent")`, `GetContentDir()`, module dependency `Projects`.
7. `ADDFloorMapActor`: `/Engine/BasicShapes/Cube`, `Sphere`, `BasicShapeMaterial` and its vector parameter named `Color`; `CreateDynamicMaterialInstance(int, UMaterialInterface*)`; `FLinearColor::MakeFromHSV8`; `SetMobility` before `SetStaticMesh`; `SetActorLabel` in editor; `CallInEditor` button on a level actor.
8. `UCLASS` pimpl: `struct FDDPuzzleImpl;` / `FDDQuestImpl` are declared at global scope in headers and defined in `.cpp` (no UPROPERTY on them).
9. `UFUNCTION(BlueprintPure)` on a `static` member in a component (`StageFromScore`) and `const` functions with `TArray` returns.
10. Reference-output parameters (`FString& Error`) show as Blueprint outputs only when the function is not `const`/pure-with-refs rules allow; adjust if the node pins look wrong.
11. Dynamic multicast delegates carrying structs by `const&` in `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam`.
