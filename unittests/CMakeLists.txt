cmake_minimum_required(VERSION 3.20)

find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
set(memcheck_command 
  ${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS}
  --error-exitcode=1 --leak-check=full --show-leak-kinds=all --track-origins=yes)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

add_subdirectory(Common)
add_subdirectory(Lexer)
add_subdirectory(Parser)
add_subdirectory(Source)
add_subdirectory(Diagnostics)
add_subdirectory(Fuzzer)
add_subdirectory(Driver)
#add_subdirectory(CppRefactor)
add_subdirectory(Testing)