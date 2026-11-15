cmake_minimum_required(VERSION 3.16)
project(gwent_classic_miyoo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(GWENT_BUILD_TESTS "Build engine tests" ON)
option(GWENT_WITH_SDL "Build the SDL front end when SDL2 is installed" ON)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(GWENT_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(GWENT_CATALOG_SOURCE ${GWENT_GENERATED_DIR}/reference_catalog.cpp)
add_custom_command(
  OUTPUT ${GWENT_CATALOG_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_catalog.py
          ${CMAKE_CURRENT_SOURCE_DIR}/data/cards.json ${GWENT_CATALOG_SOURCE}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_catalog.py ${CMAKE_CURRENT_SOURCE_DIR}/data/cards.json
  VERBATIM
)

add_library(gwent_engine src/game.cpp ${GWENT_CATALOG_SOURCE})
target_include_directories(gwent_engine PUBLIC include)
if(MSVC)
  target_compile_options(gwent_engine PRIVATE /W4 /permissive-)
else()
  target_compile_options(gwent_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(GWENT_WITH_SDL)
  find_package(SDL2 QUIET)
  if(SDL2_FOUND)
    add_executable(gwent src/main.cpp)
    target_link_libraries(gwent PRIVATE gwent_engine SDL2::SDL2)
    target_compile_definitions(gwent PRIVATE GWENT_HAS_SDL=1)
  else()
    message(STATUS "SDL2 not found: SDL front end is not built")
  endif()
endif()

if(GWENT_BUILD_TESTS)
  enable_testing()
  add_executable(gwent_engine_tests tests/engine_tests.cpp)
  target_link_libraries(gwent_engine_tests PRIVATE gwent_engine)
  add_test(NAME engine_rules COMMAND gwent_engine_tests)
  add_test(NAME card_catalog COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/validate_catalog.py ${CMAKE_SOURCE_DIR}/data)
endif()
