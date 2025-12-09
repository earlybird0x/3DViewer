cmake_minimum_required(VERSION 3.16)

project(3DViewer LANGUAGES CXX)

include(CTest)
enable_testing()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Concurrent)

if (QT_VERSION_MAJOR EQUAL 6)
  find_package(Qt6 REQUIRED COMPONENTS OpenGL OpenGLWidgets)
else()
  find_package(Qt5 REQUIRED COMPONENTS Gui OpenGL)
endif()

add_library(viewer_core STATIC
  src/model/obj_model.cpp
  src/model/obj_parser.cpp
)
target_include_directories(viewer_core PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/model
)

target_link_libraries(viewer_core PUBLIC Qt${QT_VERSION_MAJOR}::Core)

file(GLOB_RECURSE PROJECT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui
)

list(REMOVE_ITEM PROJECT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/model/obj_model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/model/obj_parser.cpp
)

add_executable(3DViewer ${PROJECT_SOURCES})

target_include_directories(3DViewer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/view
  ${CMAKE_CURRENT_SOURCE_DIR}/src/controller
  ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

if (QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(3DViewer PRIVATE
    viewer_core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Concurrent
  )
else()
  target_link_libraries(3DViewer PRIVATE
    viewer_core
    Qt5::Widgets
    Qt5::Gui
    Qt5::OpenGL
    Qt5::Concurrent
  )
endif()

if (WIN32)
  target_link_libraries(3DViewer PRIVATE opengl32)
endif()

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/googletest)
add_subdirectory(tests)
