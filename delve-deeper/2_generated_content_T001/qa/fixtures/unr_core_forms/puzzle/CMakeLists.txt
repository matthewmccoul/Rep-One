# Core library: built with add_core_library (no exceptions, no RTTI). Text in, results out; no file I/O.
add_core_library(dd_puzzle src/puzzle.cpp)
target_link_libraries(dd_puzzle PUBLIC dd_common dd_map)

# NOT core: command-line entry. cli/CMakeLists.txt is not edited by this step, so the puzzle_check
# sources (in cli/src/) are built here and link dd_cli for file reading.
add_library(dd_puzzle_cli STATIC ${CMAKE_SOURCE_DIR}/cli/src/puzzle_cli.cpp)
target_link_libraries(dd_puzzle_cli PUBLIC dd_cli dd_puzzle dd_map dd_common)
add_executable(puzzle_check ${CMAKE_SOURCE_DIR}/cli/src/puzzle_check_main.cpp)
target_link_libraries(puzzle_check PRIVATE dd_puzzle_cli)

add_executable(puzzle_tests ${CMAKE_SOURCE_DIR}/tests/puzzle/test_puzzle.cpp)
target_link_libraries(puzzle_tests PRIVATE dd_puzzle_cli)
target_compile_definitions(puzzle_tests PRIVATE DD_REPO_ROOT="${DD_REPO_ROOT}" DD_TEST_SRC="${CMAKE_SOURCE_DIR}"
                           DD_TMP_DIR="${DD_TMP_DIR}")

# One ctest entry per test case (keep in step with tests/puzzle/test_puzzle.cpp).
foreach(t
    pzl_data_real pzl_data_errors pzl_map_cross_real pzl_map_cross_broken pzl_all_orderings
    pzl_oracle_sequences pzl_repeat_and_reset pzl_solve pzl_no_lockout pzl_unknown_control
    pzl_bypass_routes pzl_bypass_after_progress pzl_second_open pzl_solve_after_bypass
    pzl_roundtrip_prefixes pzl_roundtrip_bypass pzl_persist_open pzl_xp_claim
    pzl_bad_state_text pzl_state_fixtures pzl_load_keeps_state_on_reject pzl_ref_python_cases
    pzl_strings_exist pzl_cli_functions)
  add_test(NAME ${t} COMMAND puzzle_tests ${t})
endforeach()

# Names that check_spec.md uses (PZL-14, 20, 23, 24, 28) for the same cases.
foreach(pair "puzzle_no_lockout|pzl_no_lockout" "puzzle_second_open|pzl_second_open"
        "puzzle_roundtrip_bypass|pzl_roundtrip_bypass" "puzzle_persist_open|pzl_persist_open"
        "puzzle_xp_claim|pzl_xp_claim")
  string(REPLACE "|" ";" ab "${pair}")
  list(GET ab 0 alias)
  list(GET ab 1 real)
  add_test(NAME ${alias} COMMAND puzzle_tests ${real})
endforeach()

# The real executable, run as a user would (exit code and last line).
set(PZL_PUZ ${DD_REPO_ROOT}/work/T001/puzzle/storeroom_puzzle.json)
set(PZL_MAP ${DD_REPO_ROOT}/work/T001/map/floor1_map.json)
set(PZL_FX ${CMAKE_SOURCE_DIR}/tests/puzzle/fixtures)
add_test(NAME pzl_exe_validate COMMAND puzzle_check validate ${PZL_PUZ} ${PZL_MAP})
set_tests_properties(pzl_exe_validate PROPERTIES PASS_REGULAR_EXPRESSION "OK puzzle controls=3 solution=3")
add_test(NAME pzl_exe_seq_solve COMMAND puzzle_check seq ${PZL_PUZ} ctl_pick ctl_cart ctl_furnace)
set_tests_properties(pzl_exe_seq_solve PROPERTIES PASS_REGULAR_EXPRESSION
  "state=solved door_open=1 opened_via=puzzle wrong=0 progress=ctl_pick,ctl_cart,ctl_furnace\n$")
add_test(NAME pzl_exe_seq_wrong COMMAND puzzle_check seq ${PZL_PUZ} ctl_pick ctl_furnace ctl_cart)
set_tests_properties(pzl_exe_seq_wrong PROPERTIES PASS_REGULAR_EXPRESSION
  "state=idle door_open=0 opened_via=none wrong=2 progress=\n$")
add_test(NAME pzl_exe_bypass COMMAND puzzle_check bypass ${PZL_PUZ} magic_key ctl_pick)
set_tests_properties(pzl_exe_bypass PROPERTIES PASS_REGULAR_EXPRESSION
  "ctl_pick inert str.use.inert\nstate=bypassed door_open=1 opened_via=magic_key wrong=0 progress=\n$")
add_test(NAME pzl_exe_roundtrip COMMAND puzzle_check roundtrip ${PZL_PUZ} ctl_pick ctl_cart)
set_tests_properties(pzl_exe_roundtrip PROPERTIES PASS_REGULAR_EXPRESSION
  "state=partial door_open=0 opened_via=none wrong=0 progress=ctl_pick,ctl_cart\n$")
add_test(NAME pzl_exe_load COMMAND puzzle_check load ${PZL_PUZ} ${PZL_FX}/save_partial.json)
set_tests_properties(pzl_exe_load PROPERTIES PASS_REGULAR_EXPRESSION "OK load")
# Exit code 1 cases: PASS_REGULAR_EXPRESSION would ignore the exit code, so these run through the
# ctest-side cmake script that requires exit 1 and a first line starting with FAIL.
foreach(c
    "bad_unknown_control|seq|ctl_door|FAIL UNKNOWN_CONTROL"
    "bad_unknown_route|bypass|kick|FAIL UNKNOWN_ROUTE"
    "bad_save_missing|load|${PZL_FX}/save_missing.json|FAIL SAVE_FIELD"
    "bad_save_inconsistent|load|${PZL_FX}/save_inconsistent.json|FAIL SAVE_STATE"
    "bad_save_value|load|${PZL_FX}/save_bad_via.json|FAIL SAVE_VALUE")
  string(REPLACE "|" ";" parts "${c}")
  list(GET parts 0 nm)
  list(GET parts 1 sub)
  list(GET parts 2 arg)
  list(GET parts 3 want)
  add_test(NAME pzl_exe_${nm}
           COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:puzzle_check> -DSUB=${sub} -DPUZ=${PZL_PUZ} -DARG=${arg}
                   -DWANT=${want} -P ${CMAKE_SOURCE_DIR}/tests/puzzle/expect_fail.cmake)
endforeach()
add_test(NAME pzl_exe_bad_edge
         COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:puzzle_check> -DSUB=validate -DPUZ=${PZL_FX}/bad_edge.json
                 -DARG=${PZL_MAP} -DWANT=FAIL\ PUZZLE_EDGE -P ${CMAKE_SOURCE_DIR}/tests/puzzle/expect_fail.cmake)
