cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Export compilation database in JSON format.

project(lbs LANGUAGES CXX)

add_library(libparser lib/parser/parser.cpp)
target_include_directories(libparser PUBLIC inc)

add_library(libtests lib/tests/tests.cpp)
target_include_directories(libparser PUBLIC inc)
target_link_libraries(libtests libparser)

add_executable(lbs src/main.cpp)
target_include_directories(lbs PUBLIC inc)
target_link_libraries(lbs libparser)

target_compile_options(lbs PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
