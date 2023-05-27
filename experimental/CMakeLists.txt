cmake_minimum_required(VERSION 3.20)

project(
  cocktail_experimental
  LANGUAGES C CXX
)

set(BISON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/syntax.ypp)
set(FLEX_FILE ${CMAKE_CURRENT_SOURCE_DIR}/syntax.lpp)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

BISON_TARGET(Parser ${BISON_FILE} ${CMAKE_CURRENT_BINARY_DIR}/syntax.tab.cc
                    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/syntax.tab.h VERBOSE)
FLEX_TARGET(Scanner ${FLEX_FILE} ${CMAKE_CURRENT_BINARY_DIR}/syntax.yy.cc)

ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

include_directories(..)
link_directories(${CMAKE_CURRENT_BINARY_DIR})

set(syntax_SRCS
  syntax.tab.cc
  syntax.yy.cc
)

file(GLOB_RECURSE EXPERIMENTAL_PATH
    *.cc
    AST/*.cc
    interpreter/*.cc
  )
list(APPEND syntax_SRCS ${EXPERIMENTAL_PATH})

foreach(FILE_NAME ${syntax_SRCS})
  message(STATUS "syntax_SRCS files found: ${FILE_NAME}")
endforeach()

message(STATUS "FLEX_Scanner_OUTPUTS files found: ${CMAKE_CURRENT_BINARY_DIR}/syntax.yy.cc")
message(STATUS "BISON_Parser_OUTPUTS files found: ${CMAKE_CURRENT_BINARY_DIR}/syntax.tab.cc")

add_executable(cocktail_exec
  ${CMAKE_CURRENT_BINARY_DIR}/syntax.tab.cc
  ${CMAKE_CURRENT_BINARY_DIR}/syntax.yy.cc
  ${syntax_SRCS})