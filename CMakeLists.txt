cmake_minimum_required(VERSION 3.14)

project(lbs LANGUAGES CXX)

add_library(libparser lib/parser/parser.cpp)

add_executable(lbs src/main.cpp)
target_include_directories(lbs PUBLIC inc)
target_link_libraries(lbs libparser)

target_compile_options(lbs PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
