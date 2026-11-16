# Core library: built with add_core_library (no exceptions, no RTTI). Text in, results out; no file I/O.
add_core_library(dd_common src/json.cpp src/csv.cpp)

add_executable(com_tests ${CMAKE_SOURCE_DIR}/tests/common/test_common.cpp)
target_link_libraries(com_tests PRIVATE dd_cli dd_common)
target_compile_definitions(com_tests PRIVATE DD_REPO_ROOT="${DD_REPO_ROOT}" DD_TMP_DIR="${DD_TMP_DIR}")

# One ctest entry per test case (names are the case names; keep in step with test_common.cpp).
foreach(t
    com_json_valid_real_files com_json_escapes com_json_numbers_literals com_json_trailing_comma
    com_json_truncated com_json_dup_key com_json_nonstrict com_json_empty com_json_depth
    com_json_error_paths
    com_cli_json_exit_codes com_csv_valid_real com_csv_quoted_fields com_csv_ragged
    com_csv_open_quote com_csv_header_rules)
  add_test(NAME ${t} COMMAND com_tests ${t})
endforeach()
