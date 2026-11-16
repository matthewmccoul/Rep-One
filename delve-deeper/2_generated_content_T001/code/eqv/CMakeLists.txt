# NOT core: equivalence tooling (C++ puzzle core vs the Python reference). Ordinary compile flags.
# eqv_check <golden.json> replays gen_golden.py vectors through dd_puzzle. cli/CMakeLists.txt is not edited;
# dd_cli is defined in a later add_subdirectory (module order common;map;puzzle;quest;eqv;cli), which
# target_link_libraries resolves at generate time.
add_executable(eqv_check src/eqv_check.cpp)
target_link_libraries(eqv_check PRIVATE dd_cli dd_puzzle dd_map dd_common)
target_compile_definitions(eqv_check PRIVATE
  EQV_DEFAULT_PUZZLE="${DD_REPO_ROOT}/work/T001/puzzle/storeroom_puzzle.json")

# EQV-13 (quest_transition_table) already exists: quest/CMakeLists.txt, test source
# tests/quest/test_quest.cpp, ctest name qst_quest_transition_table. Not duplicated here.

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  set(EQV_PY ${CMAKE_SOURCE_DIR}/tests/eqv/test_eqv_tools.py)
  set(EQV_SCR ${DD_TMP_DIR}/eqv)
  foreach(c
      gen_and_match gen_missing_reference gen_depends_on_reference eqv_tampered eqv_empty
      eqv_changed_reference eqv_bad_files guard_real guard_leaks guard_csv_real guard_csv_leaks)
    string(REGEX REPLACE "^eqv_" "" short ${c})
    add_test(NAME eqv_${short}
             COMMAND ${Python3_EXECUTABLE} -I ${EQV_PY} ${c} ${CMAKE_SOURCE_DIR} ${DD_REPO_ROOT}
                     $<TARGET_FILE:eqv_check> ${EQV_SCR}/${c})
  endforeach()

  # The plain command lines of the spec, run as a user would (PZL-30, EQV-02, EQV-05).
  add_test(NAME eqv_pzl_text_guard_real
           COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_SOURCE_DIR}/tools/check_text_guard.py
                   ${DD_REPO_ROOT}/work/T001/puzzle/storeroom_puzzle.json)
  # QST-17: the same plain command line on the real quest strings CSV.
  add_test(NAME eqv_qst_text_guard_real
           COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_SOURCE_DIR}/tools/check_text_guard.py
                   ${DD_REPO_ROOT}/work/T001/quest/figure_ore_lift_strings.csv)
  # QST-18: a leaking CSV fixture must fail, naming the key (regular expression decides, exit code ignored).
  add_test(NAME eqv_qst_text_guard_leak_fixture
           COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_SOURCE_DIR}/tools/check_text_guard.py
                   ${DD_REPO_ROOT}/work/T001/code/tests/quest/fixtures/q_leak_fig_offer_cold.csv)
  set_tests_properties(eqv_qst_text_guard_leak_fixture PROPERTIES
                       PASS_REGULAR_EXPRESSION "FAIL TEXT_LEAK: fig_offer_cold: contains 'because'")
  add_test(NAME eqv_gen_golden
           COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_SOURCE_DIR}/tools/gen_golden.py ${EQV_SCR}/golden.json)
  set_tests_properties(eqv_gen_golden PROPERTIES FIXTURES_SETUP eqv_golden)
  add_test(NAME eqv_check_golden COMMAND eqv_check ${EQV_SCR}/golden.json)
  set_tests_properties(eqv_check_golden PROPERTIES FIXTURES_REQUIRED eqv_golden
                       PASS_REGULAR_EXPRESSION "[0-9]+ vectors, 0 mismatches")
endif()
