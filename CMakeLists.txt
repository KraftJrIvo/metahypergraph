cmake_minimum_required(VERSION 3.15...3.27)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
set(SILENCE_ALL_CXX17_DEPRECATION_WARNINGS ON)

project(MHG VERSION 1.0 DESCRIPTION "meta hyper graph" LANGUAGES CXX)

# TARGET
set(MHG_SOURCE_FILES    
    "src/types/node.cpp"
    "src/types/edge.cpp"
    "src/types/hypergraph.cpp"
    "src/types/metahypergraph.cpp"
    "src/util/bezier.cpp"
    "src/util/floyd_warshall.cpp"
    "src/util/kamada_kawai.cpp"
    "src/drawer.cpp"
    "src/main.cpp"
    "res/icon.rc"
)
add_executable(MHG ${MHG_SOURCE_FILES})
target_include_directories(MHG PUBLIC "src")

# DEPENDENCIES
find_package(Eigen3 CONFIG REQUIRED)
target_link_libraries(MHG PUBLIC Eigen3::Eigen)

set(RAYLIB_VERSION 5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()
target_link_libraries(MHG PUBLIC raylib)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res
)
add_dependencies(raylib copy_assets)