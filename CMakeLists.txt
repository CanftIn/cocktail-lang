cmake_minimum_required(VERSION 3.20)
project(
  cocktail
  VERSION 0.0.1
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")
set(project_version "${${PROJECT_NAME}_VERSION}")

message(STATUS "Project '${PROJECT_NAME}', version: '${project_version}'")

option(COCKTAIL_OPT_BUILD_UNITTESTS "Build all cocktail unittests" ON)
option(COCKTAIL_OPT_BUILD_BENCHMARKS "Build all cocktail benchmarks" ON)
option(COCKTAIL_OPT_BUILD_TOOLS "Build cocktail execute tools" ON)
option(COCKTAIL_OPT_BUILD_EXPERIMENTAL "Build cocktail experimental implementation" ON)

# temp define: https://discourse.llvm.org/t/python-api-problem/945
add_compile_options(-fno-rtti)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # for debug of stl structure while using clang compile
  add_compile_options(-fstandalone-debug)
endif()

#------------------------------------------------------------
# LLVM BEGIN
#------------------------------------------------------------

# apt install llvm-15-dev libclang-15-dev
set(LLVM_DIR /usr/lib/llvm-15/lib/cmake/llvm)
set(Clang_DIR /usr/lib/llvm-15/lib/cmake/clang)
# manually install
# set(LLVM_DIR /usr/local/lib/cmake/llvm)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
#------------------------------------------------------------
# LLVM END
#------------------------------------------------------------

#------------------------------------------------------------
# CMake helpers begin
#------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(CTest)
enable_testing()
find_package(GTest REQUIRED CONFIG)

include_directories(include)

#------------------------------------------------------------
# CMake helpers end
#------------------------------------------------------------

#------------------------------------------------------------
# lib make begin
#------------------------------------------------------------

# Create the cocktail C++ library
set(COCKTAIL_SRCS)
set(COCKTAIL_COMMON_SRCS)
set(COCKTAIL_LEX_SRCS)
set(COCKTAIL_PARSE_SRCS)
set(COCKTAIL_AST_SRCS)

set(STATIC_LIB_NAME ${PROJECT_NAME})
set(COCKTAIL_COMMON_LIB ${PROJECT_NAME}Common)
set(COCKTAIL_LEX_LIB ${PROJECT_NAME}Lex)
set(COCKTAIL_PARSE_LIB ${PROJECT_NAME}Parse)
set(COCKTAIL_AST_LIB ${PROJECT_NAME}AST)

set(STATIC_LIB_NAME ${PROJECT_NAME})

# cocktailCommon lib
file(GLOB_RECURSE COCKTAIL_COMMON_SRCS
    lib/Common/*.cc
  )
add_library(${COCKTAIL_COMMON_LIB} STATIC ${COCKTAIL_COMMON_SRCS})
target_link_libraries(${COCKTAIL_COMMON_LIB}
  LLVMSupport
)

# # cocktailParse lib
# file(GLOB_RECURSE LIB_PARSE_PATH
#     ./lib/Parse/*.cc
#   )
# list(APPEND COCKTAIL_PARSE_SRCS ${LIB_PARSE_PATH})
# add_library(${COCKTAIL_PARSE_LIB} STATIC ${COCKTAIL_PARSE_SRCS})
# target_link_libraries(${COCKTAIL_PARSE_LIB}
#   LLVMSupport
#   clangBasic
# )
# 
# # cocktailAST lib begin
# file(GLOB_RECURSE LIB_AST_PATH
#     ./lib/AST/*.cc
#   )
# list(APPEND COCKTAIL_AST_SRCS ${LIB_AST_PATH})
# add_library(${COCKTAIL_AST_LIB} STATIC ${COCKTAIL_AST_SRCS})
# target_link_libraries(${COCKTAIL_AST_LIB}
#   LLVMSupport
# )

#------------------------------------------------------------
# lib make end
#------------------------------------------------------------

if (COCKTAIL_OPT_BUILD_UNITTESTS)
  add_subdirectory(unittests #[[EXCLUDE_FROM_ALL]])
endif()

# if (COCKTAIL_OPT_BUILD_BENCHMARKS)
#   add_subdirectory(benchmarks #[[EXCLUDE_FROM_ALL]])
# endif()
# 
# if (COCKTAIL_OPT_BUILD_TOOLS)
#   add_subdirectory(tools)
# endif()
# 
# if (COCKTAIL_OPT_BUILD_EXPERIMENTAL)
#   add_subdirectory(experimental)
# endif()