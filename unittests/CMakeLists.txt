cmake_minimum_required(VERSION 3.6)

find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
set(memcheck_command ${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS} --error-exitcode=1 --leak-check=full)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

enable_testing()
find_package(GTest REQUIRED)

file(GLOB UNITTESTS_LIST *.cc)

foreach(FILE_PATH ${UNITTESTS_LIST})
  STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
  message(STATUS "unittest files found: ${FILE_NAME}.cc")
  add_executable(${FILE_NAME} ${FILE_NAME}.cc)
  target_link_libraries(${FILE_NAME} GTest::gtest GTest::gtest_main cocktail)
  add_test(${FILE_NAME} ${FILE_NAME})
  add_dependencies(check ${FILE_NAME})
  add_test(${FILE_NAME}-memory-check ${memcheck_command} ./${FILE_NAME})
endforeach()