# NOT core: file reading and the command-line tools. Ordinary compile flags (exceptions allowed here);
# links against the core libraries exactly as the core is built.
add_library(dd_cli STATIC src/fileio.cpp src/map_cli.cpp)
target_include_directories(dd_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dd_cli PUBLIC dd_common dd_map)

add_executable(com_check src/com_check_main.cpp)
target_link_libraries(com_check PRIVATE dd_cli)

add_executable(map_check src/map_check_main.cpp)
target_link_libraries(map_check PRIVATE dd_cli)

# CLI on the real files (exit 0 expected).
add_test(NAME com_cli_real_map COMMAND com_check json ${DD_REPO_ROOT}/work/T001/map/floor1_map.json)
add_test(NAME com_cli_real_csv COMMAND com_check csv ${DD_REPO_ROOT}/work/T001/quest/figure_ore_lift_strings.csv)
add_test(NAME map_cli_real_validate COMMAND map_check validate ${DD_REPO_ROOT}/work/T001/map/floor1_map.json)
