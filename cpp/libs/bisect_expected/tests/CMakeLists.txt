project(bisect_expected_tests LANGUAGES CXX)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_source_files})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE bisect::project_options bisect::project_warnings bisect::expected gtest::gtest)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
