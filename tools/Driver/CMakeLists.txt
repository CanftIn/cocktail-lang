cmake_minimum_required(VERSION 3.6)

find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
set(memcheck_command ${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS} --error-exitcode=1 --leak-check=full)

file(GLOB DRIVER_LIST *.cc)

foreach(FILE_PATH ${DRIVER_LIST})
  STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
  message(STATUS "driver files found: ${FILE_NAME}.cc")
  add_executable(${FILE_NAME} ${FILE_NAME}.cc)
  target_link_libraries(${FILE_NAME} cocktail)
  add_dependencies(check ${FILE_NAME})
  add_test(${FILE_NAME}-memory-check ${memcheck_command} ./${FILE_NAME})
endforeach()
