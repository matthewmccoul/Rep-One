cmake_minimum_required(VERSION 3.16)
project(DelveDeeperCore CXX)

# Plain C++17, standard library only. No Unreal includes anywhere in this tree's core modules.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra)
endif()

# add_core_library(<name> <sources...>)
# EVERY core module (common, map, puzzle, quest, ...) must be created with this helper. Unreal modules
# compile with C++ exceptions and RTTI off, so core code is built the same way here:
#   -fno-exceptions -fno-rtti (GCC/Clang)  /  /EHs-c- /GR- (MSVC)
# It also sets the include rule: public headers live in <module>/include (include "dd/<header>.h").
# Test programs and the cli/ tools are NOT core and use plain add_library/add_executable, but they
# link the core libraries as built here.
function(add_core_library name)
  add_library(${name} STATIC ${ARGN})
  target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${name} PRIVATE -fno-exceptions -fno-rtti)
  elseif(MSVC)
    target_compile_options(${name} PRIVATE /EHs-c- /GR-)
  endif()
  set_property(GLOBAL APPEND PROPERTY DD_CORE_LIBRARIES ${name})
endfunction()

# All executables land directly in the build directory (work/T001/build/).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Repo root (work/T001/code -> three levels up) and a scratch dir for test-written fixtures.
get_filename_component(DD_REPO_ROOT "${CMAKE_SOURCE_DIR}/../../.." ABSOLUTE)
set(DD_TMP_DIR "${CMAKE_BINARY_DIR}/tmp_tests")

enable_testing()

# Core modules first, then the non-core cli/ (it links them).
# A step may build only its own modules: cmake -DDD_MODULES="common;map;puzzle;cli" ...
set(DD_MODULES "common;map;puzzle;quest;eqv;cli" CACHE STRING "core modules to build, in order")
foreach(mod ${DD_MODULES})
  if(EXISTS "${CMAKE_SOURCE_DIR}/${mod}/CMakeLists.txt")
    add_subdirectory(${mod})
  endif()
endforeach()

# Purity checker as a ctest test (Unreal includes/macros and forbidden constructs in core code).
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME unr_core_purity
           COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_SOURCE_DIR}/tools/check_no_unreal.py ${CMAKE_SOURCE_DIR}
                   --log ${CMAKE_SOURCE_DIR}/logs/check_no_unreal.log)
  add_test(NAME unr_cmake_no_unreal
           COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_SOURCE_DIR}/tools/check_no_unreal.py --cmake ${CMAKE_SOURCE_DIR})
endif()
