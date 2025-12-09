set(target 3DViewer_tests)

set(TEST_CANDIDATES
  test_model_edges_aabb.cpp
  test_model_transform.cpp
  test_obj_parser.cpp
)

set(TEST_SOURCES "")
foreach(f IN LISTS TEST_CANDIDATES)
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
    list(APPEND TEST_SOURCES "${f}")
  else()
    message(STATUS "Test file not found (skipped): ${f}")
  endif()
endforeach()

if (TEST_SOURCES STREQUAL "")
  message(FATAL_ERROR "No test sources found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

add_executable(${target} ${TEST_SOURCES})

target_include_directories(${target} PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/model
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${target} PRIVATE
  viewer_core
  GTest::gtest_main
)

if (WIN32)
  if (CMAKE_PREFIX_PATH)
    list(GET CMAKE_PREFIX_PATH 0 QT_PREFIX_HINT)
  endif()
  set(QT_BIN_DIR "${QT_PREFIX_HINT}/bin")
  set(MINGW_BIN_DIR "C:/Qt/Tools/mingw1310_64/bin")
  set(TEST_ENV_PATH "${QT_BIN_DIR};${MINGW_BIN_DIR};$ENV{PATH}")
endif()

include(GoogleTest)
gtest_discover_tests(${target}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DISCOVERY_TIMEOUT 60
  DISCOVERY_MODE PRE_TEST
)
