# s013-software-engineer-unreal: Unreal wrapper source and plugin (owner: development-software-engineer)
Task ID T001-s014-software. Written by the Producer (the chat session). Build step 4b of 4 for the C++ package. Step 4a (equivalence tooling) is built IN PARALLEL by another run: keep to your own files (rules below).

## Goal
The user asked for the three pieces as functional code that runs inside Unreal Engine. The tested core (plain C++17, no Unreal includes) is done for the map, the puzzle and the quest. Now write the thin Unreal side that calls it, packaged as a plugin the user can drop into an Unreal project. Unreal Engine is NOT installed here, so none of this can be compiled or run here: every claim about it is NOT RUN, and you must say so plainly. Use WebSearch to check Unreal API names and the plugin and Build.cs conventions against current Unreal documentation, and state the engine version you targeted (assume UE 5.4 unless the documentation tells you a safer choice).

## What to build under work/T001/code/unreal/
- A plugin folder `DelveDeeperContent/` with the `.uplugin`, `Source/DelveDeeperContent/` (`DelveDeeperContent.Build.cs` with exceptions off, RTTI off and C++17; `Public/`, `Private/`), and `Content/Data/` for the data files.
- Thin wrapper classes, each a `UCLASS`: a puzzle component (UActorComponent) that wraps the core puzzle (Use on a control, bypass routes, save and load state as text), a quest component (UActorComponent) that wraps the core quest (offer, accept, decline, observe, report, the line to show, reward through a supplied roll), and a map facility (for example a `UBlueprintFunctionLibrary` plus an actor that reads the floor map and spawns simple placeholder actors for rooms, doors and encounter or puzzle sites so the floor can be seen and walked through in the editor). Expose what a designer needs to Blueprints (`UFUNCTION(BlueprintCallable)`, events for what the player should see).
- Wrappers hold NO game rules and NO content: no control ids, edge ids, quest keys or order comparisons in wrapper code. They convert `FString` to and from `std::string`, load the data files as text (for example `FFileHelper::LoadFileToString` from the plugin's `Content/Data/`), call the core, and translate results into Unreal types and events.
- The core cannot be referenced across directories by Unreal's build system, so write `work/T001/code/tools/sync_core_into_plugin.py`: it copies the core headers and sources (`common`, `map`, `puzzle`, `quest`) into `DelveDeeperContent/Source/DelveDeeperContent/Private/Core/` and `Public/Core/` (generated copies, marked with a header comment saying generated, do not edit) and the data files (`work/T001/map/floor1_map.json`, `work/T001/puzzle/storeroom_puzzle.json`, `work/T001/quest/figure_ore_lift_quest.json`, `figure_ore_lift_strings.csv`) into `Content/Data/`. Run it so the plugin folder is complete. It must be repeatable and must fail loudly if a source is missing.
- `work/T001/code/tools/check_unreal_wrapper.py <dir>`: the static wrapper checker QA specified (UNR-08, UNR-09, UNR-10 in `work/T001/qa/check_spec.md`, Section 5): scans the hand-written wrapper files (not the generated `Private/Core/` and `Public/Core/` copies), lists each wrapper class and the core calls it makes, and exits 1 on a wrapper that hard-codes game content or rules, or that makes no call into the core. Implement exactly as the spec says, including its broken-input behaviour. Run it on your tree and report its real output.
- `work/T001/code/unreal/README_unreal.md`: how to install the plugin in a project, the engine version targeted, each class and what it exposes, how the data and the core copies get there (the sync script), a plain list of what is NOT RUN, and the assumptions about Unreal APIs that a human with Unreal should check first when compiling.

## Rules
- The `unreal/` folder must NOT be part of the CMake build (UNR-06). Do not edit any CMake file.
- Hand-written wrapper files include only core headers (project-rooted, through the plugin's include paths) and Unreal headers. Keep each wrapper thin.
- The generated core copies under the plugin must be identical to the real core files except for the generated-file banner; the sync script prints what it copied.
- Do not claim anything compiled. The checker and the sync script are the only things you can run.

## Files you must NOT edit (shared)
Top-level `CMakeLists.txt`, `cli/`, `common/`, `map/`, `puzzle/`, `quest/`, `eqv/` (the parallel run), `tests/`, `README_code.md`, and `tools/` files other than the two new scripts named above.

## Acceptance
- The plugin folder exists and is complete (uplugin, Build.cs, wrappers, generated core copies, data files); the sync script ran and reported what it copied; the wrapper checker exits 0 on it and lists each wrapper class with core calls greater than zero.
- No game rule or content id appears in a hand-written wrapper file.
- `README_unreal.md` states what is NOT RUN and names the first things a human should check in Unreal.

## Stop limit
One pass. Write only under `work/T001/code/unreal/`, the two new scripts in `work/T001/code/tools/`, and `work/T001/code/logs/`. End with STATUS, OUTPUTS, ISSUES. Evidence: EXECUTED (with log path) only for the two scripts; NOT RUN for everything that needs Unreal.
