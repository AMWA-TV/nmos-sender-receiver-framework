project(bisect_expected LANGUAGES CXX)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

find_package(fmt REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)
add_library(bisect::expected ALIAS ${PROJECT_NAME})

target_link_libraries(
        ${PROJECT_NAME}
        INTERFACE bisect::project_options bisect::project_warnings
        fmt::fmt)

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE
  include
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
  POSITION_INDEPENDENT_CODE ON)
