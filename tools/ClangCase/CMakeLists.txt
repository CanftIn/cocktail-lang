cmake_minimum_required(VERSION 3.6)

file(GLOB DRIVER_LIST *.cc)

foreach(FILE_PATH ${DRIVER_LIST})
  STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
  message(STATUS "cpp refactor files found: ${FILE_NAME}.cc")
  add_executable(${FILE_NAME} ${FILE_NAME}.cc)
  target_link_libraries(${FILE_NAME} cocktail)
endforeach()
