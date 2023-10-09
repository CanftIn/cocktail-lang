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

# simd ssse3
add_compile_options(-mssse3)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # for debug of stl structure while using clang compile
  add_compile_options(-fstandalone-debug)
endif()

#------------------------------------------------------------
# LLVM BEGIN
#------------------------------------------------------------

# apt install llvm-15-dev libclang-15-dev
# set(LLVM_DIR /usr/lib/llvm-15/lib/cmake/llvm)
# set(Clang_DIR /usr/lib/llvm-15/lib/cmake/clang)
# manually install
set(LLVM_DIR /usr/local/lib/cmake/llvm)
#set(Clang_DIR /usr/local/lib/cmake/clang)

find_package(LLVM REQUIRED CONFIG)
#find_package(Clang REQUIRED CONFIG)

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
set(COCKTAIL_DIAGNOSTICS_SRCS)
set(COCKTAIL_SOURCE_SRCS)
set(COCKTAIL_LEX_SRCS)
set(COCKTAIL_PARSE_SRCS)
set(COCKTAIL_SEMIR_SRCS)
set(COCKTAIL_CHECK_SRCS)
set(COCKTAIL_LOWER_SRCS)
set(COCKTAIL_CODEGEN_SRCS)
set(COCKTAIL_DRIVER_SRCS)

set(STATIC_LIB_NAME ${PROJECT_NAME})
set(COCKTAIL_COMMON_LIB ${PROJECT_NAME}Common)
set(COCKTAIL_DIAGNOSTICS_LIB ${PROJECT_NAME}Diagnostics)
set(COCKTAIL_SOURCE_LIB ${PROJECT_NAME}Source)
set(COCKTAIL_LEX_LIB ${PROJECT_NAME}Lex)
set(COCKTAIL_PARSE_LIB ${PROJECT_NAME}Parse)
set(COCKTAIL_SEMIR_LIB ${PROJECT_NAME}Semir)
set(COCKTAIL_CHECK_LIB ${PROJECT_NAME}Check)
set(COCKTAIL_LOWER_LIB ${PROJECT_NAME}Lower)
set(COCKTAIL_CODEGEN_LIB ${PROJECT_NAME}CodeGen)
set(COCKTAIL_DRIVER_LIB ${PROJECT_NAME}Driver)

set(STATIC_LIB_NAME ${PROJECT_NAME})

# cocktailCommon lib
file(GLOB_RECURSE COCKTAIL_COMMON_SRCS
    lib/Common/*.cc
  )
add_library(${COCKTAIL_COMMON_LIB} STATIC ${COCKTAIL_COMMON_SRCS})
target_link_libraries(${COCKTAIL_COMMON_LIB}
  LLVMSupport
)

# cocktailDiagnostics lib
file(GLOB_RECURSE COCKTAIL_DIAGNOSTICS_SRCS
    lib/Diagnostics/*.cc
  )
add_library(${COCKTAIL_DIAGNOSTICS_LIB} STATIC ${COCKTAIL_DIAGNOSTICS_SRCS})
target_link_libraries(${COCKTAIL_DIAGNOSTICS_LIB}
  LLVMSupport
  cocktailCommon
)

# cocktailSource lib
file(GLOB_RECURSE COCKTAIL_SOURCE_SRCS
    lib/Source/*.cc
  )
add_library(${COCKTAIL_SOURCE_LIB} STATIC ${COCKTAIL_SOURCE_SRCS})
target_link_libraries(${COCKTAIL_SOURCE_LIB}
  LLVMSupport
  cocktailDiagnostics
)

# cocktailLex lib
file(GLOB_RECURSE COCKTAIL_LEX_SRCS
    lib/Lex/*.cc
  )
add_library(${COCKTAIL_LEX_LIB} STATIC ${COCKTAIL_LEX_SRCS})
target_link_libraries(${COCKTAIL_LEX_LIB}
  LLVMSupport
  cocktailDiagnostics
  cocktailCommon
  cocktailSource
)

# cocktailParse lib
file(GLOB_RECURSE LIB_PARSE_PATH
    ./lib/Parse/*.cc
  )
list(APPEND COCKTAIL_PARSE_SRCS ${LIB_PARSE_PATH})
add_library(${COCKTAIL_PARSE_LIB} STATIC ${COCKTAIL_PARSE_SRCS})
target_link_libraries(${COCKTAIL_PARSE_LIB}
  LLVMSupport
  cocktailDiagnostics
  cocktailCommon
  cocktailLex
)

# cocktailSemir lib
file(GLOB_RECURSE LIB_SEMIR_PATH
    ./lib/SemIR/*.cc
  )
list(APPEND COCKTAIL_SEMIR_SRCS ${LIB_SEMIR_PATH})
add_library(${COCKTAIL_SEMIR_LIB} STATIC ${COCKTAIL_SEMIR_SRCS})
target_link_libraries(${COCKTAIL_SEMIR_LIB}
  LLVMSupport
  LLVMCore
  cocktailDiagnostics
  cocktailCommon
  cocktailLex
  cocktailParse
)

# cocktailCheck lib
file(GLOB_RECURSE LIB_CHECK_PATH
    ./lib/Check/*.cc
  )
list(APPEND COCKTAIL_CHECK_SRCS ${LIB_CHECK_PATH})
add_library(${COCKTAIL_CHECK_LIB} STATIC ${COCKTAIL_CHECK_SRCS})
target_link_libraries(${COCKTAIL_CHECK_LIB}
  LLVMSupport
  cocktailDiagnostics
  cocktailCommon
  cocktailLex
  cocktailParse
  cocktailSemir
)

# cocktailLower lib
file(GLOB_RECURSE LIB_LOWER_PATH
    ./lib/Lower/*.cc
  )
list(APPEND COCKTAIL_LOWER_SRCS ${LIB_LOWER_PATH})
add_library(${COCKTAIL_LOWER_LIB} STATIC ${COCKTAIL_LOWER_SRCS})
target_link_libraries(${COCKTAIL_LOWER_LIB}
  LLVMSupport
  cocktailDiagnostics
  cocktailCommon
  cocktailLex
  cocktailParse
  cocktailSemir
)

# cocktailCodeGen lib
file(GLOB_RECURSE LIB_CODEGEN_PATH
    ./lib/CodeGen/*.cc
  )
list(APPEND COCKTAIL_CODEGEN_SRCS ${LIB_CODEGEN_PATH})
add_library(${COCKTAIL_CODEGEN_LIB} STATIC ${COCKTAIL_CODEGEN_SRCS})
target_link_libraries(${COCKTAIL_CODEGEN_LIB}
LLVMCore
LLVMTargetParser
LLVMObject
LLVMSupport
  cocktailDiagnostics
  cocktailCommon
  cocktailLex
  cocktailParse
  cocktailSemir
)

# cocktailDriver lib
file(GLOB_RECURSE LIB_DRIVER_PATH
    ./lib/Driver/*.cc
  )
list(APPEND COCKTAIL_DRIVER_SRCS ${LIB_DRIVER_PATH})
add_library(${COCKTAIL_DRIVER_LIB} STATIC ${COCKTAIL_DRIVER_SRCS})
target_link_libraries(${COCKTAIL_DRIVER_LIB}
  LLVMSupport
  cocktailDiagnostics
  cocktailCommon
  cocktailLex
  cocktailParse
  cocktailSemir
  cocktailLower
  cocktailCodeGen
)

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