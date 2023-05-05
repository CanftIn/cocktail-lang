cmake_minimum_required(VERSION 3.6)
project(
  cocktail-lang
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

# temp define: https://discourse.llvm.org/t/python-api-problem/945
add_compile_options(-fno-rtti)

# CMake helpers:
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(CTest)

include_directories(include)

# Create the domino C++ library
set(cocktail_SRCS)

set(STATIC_LIB_NAME ${PROJECT_NAME})

set(LLVM_DIR /usr/lib/llvm-15/lib/cmake/llvm)
find_package(LLVM REQUIRED CONFIG)

list(APPEND domino_SRCS)

add_library(${STATIC_LIB_NAME} STATIC ${domino_SRCS})
target_link_libraries(${STATIC_LIB_NAME}
  LLVMSupport
  LLVMOption
)

if (DOMINO_OPT_BUILD_UNITTESTS)
  add_subdirectory(unittests #[[EXCLUDE_FROM_ALL]])
endif()
