cmake_minimum_required (VERSION 2.8)

if (NOT DEFINED CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif ()

project (wclang)


include (CheckIncludeFiles)
check_include_files (sys/types.h HAVE_SYS_TYPES_H)
check_include_files (sys/stat.h HAVE_SYS_STAT_H)
check_include_files (string.h HAVE_STRING_H)
check_include_files (strings.h HAVE_STRINGS_H)
check_include_files (unistd.h HAVE_UNISTD_H)
check_include_files (inttypes.h HAVE_INTTYPES_H)
check_include_files (memory.h HAVE_MEMORY_H)
check_include_files (stdint.h HAVE_STDINT_H)
check_include_files (stdlib.h HAVE_STDLIB_H)


include (CheckFunctionExists)
check_function_exists (getpid HAVE_GETPID)
check_function_exists (unsetenv HAVE_UNSETENV)
check_function_exists (strrchr HAVE_STRRCHR)
check_function_exists (strchr HAVE_STRCHR)
check_function_exists (strdup HAVE_STRDUP)
check_function_exists (fork HAVE_FORK)
check_function_exists (execvp HAVE_EXECVP)
check_function_exists (getenv HAVE_GETENV)
check_function_exists (setenv HAVE_SETENV)
check_function_exists (gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists (opendir HAVE_OPENDIR)


include (CheckCXXCompilerFlag)
set (WARNING_FLAGS "-pedantic -Wall -Wextra -Wno-unused-parameter")
check_cxx_compiler_flag("${WARNING_FLAGS}" _SUPPORT_WARNING_FLAGS)
if (_SUPPORT_WARNING_FLAGS)
  set (CMAKE_CXX_FLAGS "${WARNING_FLAGS} ${CMAKE_CXX_FLAGS}")
endif ()

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package (CXX11 REQUIRED)
set (HAVE_CXX11 ${CXX11_FOUND})
set (CMAKE_CXX_FLAGS "${CXX11_FLAGS} ${CMAKE_CXX_FLAGS}")

include (CheckCXXSourceCompiles)
check_cxx_source_compiles ("#include <chrono>
int main() {
  std::chrono::steady_clock::now();
}" HAVE_STD_CHRONO)


set (PACKAGE_NAME ${PROJECT_NAME})
set (PACKAGE_BUGREPORT t.poechtrager@gmail.com)
set (PACKAGE_VERSION 0.5)
set (VERSION "${PROJECT_NAME} 0.5")


find_program (CLANG_C_COMPILER NAMES clang)
if (CLANG_C_COMPILER)
  message (STATUS "Found clang: ${CLANG_C_COMPILER}")
else ()
  message (SEND_ERROR "clang was not found")
endif ()
execute_process ( COMMAND ${CLANG_C_COMPILER} --version
                  OUTPUT_VARIABLE CLANG_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                )
string (REGEX REPLACE ".*version ([0-9\\.]+) .*"
         "\\1" CLANG_VERSION
         ${CLANG_VERSION})
if (CLANG_VERSION VERSION_GREATER 3.0)
  set (CLANG_TARGET_OPT_FLAG -target)
else ()
  set (CLANG_TARGET_OPT_FLAG -ccc-host-triple)
endif ()
add_definitions (-DCLANG_TARGET_OPT=\"${CLANG_TARGET_OPT_FLAG}\")
get_filename_component(CLANG_PATH ${CLANG_C_COMPILER} PATH)

set (TRIPLETS i686-w64-mingw32 x86_64-w64-mingw32)
list (APPEND TRIPLETS i686-w64-mingw32.static x86_64-w64-mingw32.static)
list (APPEND TRIPLETS i486-mingw32 i586-mingw32)
list (APPEND TRIPLETS i586-mingw32msvc amd64-mingw32msvc)

set (VALID_TRIPLETS)
set (MINGW_PATHS_DEF)
foreach (TRIPLET ${TRIPLETS})
  unset (MINGW_C_COMPILER CACHE)
  find_program (MINGW_C_COMPILER NAMES ${TRIPLET}-gcc)
  if (MINGW_C_COMPILER)
    get_filename_component (MINGW_C_COMPILER_REALPATH ${MINGW_C_COMPILER} REALPATH)
    get_filename_component (MINGW_PATH ${MINGW_C_COMPILER_REALPATH} PATH)
    if (MINGW_PATHS_DEF)
      set (MINGW_PATHS_DEF "${MINGW_PATHS_DEF}:${MINGW_PATH}")
    else ()
      set (MINGW_PATHS_DEF ${MINGW_PATH})
    endif ()
    message (STATUS "Found mingw-gcc: ${MINGW_C_COMPILER}")
    if (NOT MINGW_PATH STREQUAL CLANG_PATH)
      message (SEND_ERROR "mingw-gcc found (${MINGW_C_COMPILER}) but is not in the same directory as the clang binary (${CLANG_C_COMPILER}). Please run 'ln -sf ${MINGW_C_COMPILER_REALPATH} ${CLANG_PATH}' to fix this issue.")
    endif ()
    list (APPEND VALID_TRIPLETS ${TRIPLET})
  endif ()
endforeach ()
if (NOT VALID_TRIPLETS)
  message (SEND_ERROR "mingw-gcc was not found")
endif ()
add_definitions(-DMINGW_PATH=\"${MINGW_PATHS_DEF}\")


configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories (${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory (src)



set (CPACK_SOURCE_GENERATOR      "TGZ;TBZ2"     )
set (CPACK_SOURCE_IGNORE_FILES "/.git;/build;.*~;${CPACK_SOURCE_IGNORE_FILES}")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}")
include (CPack)
