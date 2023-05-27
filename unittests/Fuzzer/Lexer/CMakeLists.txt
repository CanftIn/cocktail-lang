cmake_minimum_required(VERSION 3.20)

file(GLOB FUZZER_LIST *.cc)

foreach(FILE_PATH ${FUZZER_LIST})
  STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
  message(STATUS "fuzzer files found: ${FILE_NAME}.cc")
  add_executable(${FILE_NAME} ${FILE_NAME}.cc)
  target_link_libraries(${FILE_NAME} GTest::gtest GTest::gtest_main GTest::gmock_main cocktail LLVMFuzzerCLI)
  add_test(${FILE_NAME} ${FILE_NAME} ./fuzzer_corpus/tokenized_buffer)
endforeach()