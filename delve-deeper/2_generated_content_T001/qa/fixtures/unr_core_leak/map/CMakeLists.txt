# Core library: built with add_core_library (no exceptions, no RTTI). Text in, results out; no file I/O.
add_core_library(dd_map src/map.cpp)
target_link_libraries(dd_map PUBLIC dd_common)

add_executable(map_tests ${CMAKE_SOURCE_DIR}/tests/map/test_map.cpp)
target_link_libraries(map_tests PRIVATE dd_cli dd_map)
target_compile_definitions(map_tests PRIVATE DD_REPO_ROOT="${DD_REPO_ROOT}" DD_TMP_DIR="${DD_TMP_DIR}")

# One ctest entry per test case (keep in step with test_map.cpp).
foreach(t
    map_real_validates map_dup_ids map_bad_door_type map_unknown_refs map_locked_door_rules
    map_stuck_door_rules map_secret_door_rules map_descent_rules map_orphan_rooms map_puzzle_rules
    map_count_rules map_cell_clash map_cells map_neighbours map_door_ways map_reach map_cli_failures
    map_schema_errors)
  add_test(NAME ${t} COMMAND map_tests ${t})
endforeach()
