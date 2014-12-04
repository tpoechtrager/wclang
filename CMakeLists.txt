cmake_minimum_required(VERSION 2.8)
project(wclang)


include(CheckFunctionExists)
check_function_exists(getpid HAVE_GETPID) 
check_function_exists(unsetenv HAVE_UNSETENV)

include(CheckIncludeFiles)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(CXX11 REQUIRED)
set (HAVE_CXX11 ${CXX11_FOUND})
set(CMAKE_CXX_FLAGS "${CXX11_FLAGS} ${CMAKE_CXX_FLAGS}")

INCLUDE (CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("#include <chrono>
int main() {
  std::chrono::steady_clock::now();
}" HAVE_STD_CHRONO)


set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_BUGREPORT t.poechtrager@gmail.com)
set(PACKAGE_VERSION 0.5)
set(VERSION "${PROJECT_NAME} 0.5")


find_program(CLANG_C_COMPILER NAMES clang)
if (CLANG_C_COMPILER)
  message (STATUS "Found clang: ${CLANG_C_COMPILER}")
else ()
  message (SEND_ERROR "clang was not found")
endif ()
add_definitions(-DCLANG_TARGET_OPT=\"-target\")
get_filename_component(CLANG_PATH ${CLANG_C_COMPILER} DIRECTORY)

set(TRIPLETS i686-w64-mingw32 x86_64-w64-mingw32)
list(APPEND TRIPLETS i686-w64-mingw32.static x86_64-w64-mingw32.static)
list(APPEND TRIPLETS i486-mingw32 i586-mingw32)
list(APPEND TRIPLETS i586-mingw32msvc amd64-mingw32msvc)

set (VALID_TRIPLETS)
foreach(TRIPLET ${TRIPLETS})
  unset(MINGW_C_COMPILER CACHE)
  find_program(MINGW_C_COMPILER NAMES ${TRIPLET}-gcc)
  if(MINGW_C_COMPILER)
    get_filename_component(MINGW_PATH ${MINGW_C_COMPILER} DIRECTORY)
    message(STATUS "Found mingw-gcc: ${MINGW_C_COMPILER}")
    if (NOT MINGW_PATH STREQUAL CLANG_PATH)
      message (WARNING "mingw-gcc found (${MINGW_C_COMPILER}) but is not in the same directory as the clang binary (${CLANG_C_COMPILER})")
    endif ()
    list (APPEND VALID_TRIPLETS ${TRIPLET})
  endif ()
endforeach ()


CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)

