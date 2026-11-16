# Core library: built with add_core_library (no exceptions, no RTTI). Text in, results out; no file I/O.
add_core_library(dd_quest src/quest.cpp)
target_link_libraries(dd_quest PUBLIC dd_map dd_common)

# Command-line check tool (NOT core). Its source lives in cli/src; cli/CMakeLists.txt is not edited.
add_executable(quest_check ${CMAKE_SOURCE_DIR}/cli/src/quest_check_main.cpp)
target_link_libraries(quest_check PRIVATE dd_cli dd_quest dd_map dd_common)

add_executable(quest_tests ${CMAKE_SOURCE_DIR}/tests/quest/test_quest.cpp)
target_link_libraries(quest_tests PRIVATE dd_cli dd_quest dd_map dd_common)
target_compile_definitions(quest_tests PRIVATE DD_REPO_ROOT="${DD_REPO_ROOT}" DD_TMP_DIR="${DD_TMP_DIR}"
                           QUEST_CHECK_EXE="$<TARGET_FILE:quest_check>")
add_dependencies(quest_tests quest_check)

# One ctest entry per test case, named qst_<case> (every case name starts with "quest_", so the QA
# patterns -R quest_roundtrip, quest_hostile, quest_fill_all, ... match too). Keep in step with test_quest.cpp.
foreach(t
    quest_real_validates quest_stage_bands quest_dialogue_agrees_csv quest_fill_all quest_fill_rules
    quest_offer_variants quest_accept_decline quest_arrive_variants quest_report_variants quest_per_character
    quest_hostile_midquest quest_hostile_reward quest_reward_once quest_reward_roll
    quest_roundtrip quest_load_invalid quest_id_checks quest_csv_rules quest_coverage_rule
    quest_transition_table quest_cli_validate quest_cli_stage_line_fill quest_cli_run quest_cli_hostile
    quest_cli_reward)
  add_test(NAME qst_${t} COMMAND quest_tests ${t})
endforeach()

# The Python reference validator on the real files (exit 0 expected; arguments given, so it writes no log).
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME qst_quest_python_reference
           COMMAND ${Python3_EXECUTABLE} -I ${DD_REPO_ROOT}/work/T001/quest/validate_quest.py
                   ${DD_REPO_ROOT}/work/T001/quest ${DD_REPO_ROOT}/work/T001/map/floor1_map.json)
endif()
