file(GLOB UNITTESTS_LIST *.cc)

foreach(FILE_PATH ${UNITTESTS_LIST})
  STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
  message(STATUS "unittest files found: ${FILE_NAME}.cc")
  add_executable(${FILE_NAME} ${FILE_NAME}.cc)
  target_link_libraries(${FILE_NAME}
      GTest::gtest
      GTest::gtest_main
      GTest::gmock_main
      cocktailCommon
    )
  add_test(${FILE_NAME} ${FILE_NAME})
endforeach()