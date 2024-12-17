cmake_minimum_required(VERSION 3.16)
project(ossrf_c_nmos_api LANGUAGES CXX C)

find_package(nlohmann_json REQUIRED)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_source_files})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE project_options project_warnings
        PUBLIC
        bisect::project_warnings
        bisect::expected
        bisect::bisect_nmoscpp
        bisect::bisect_json
        nlohmann_json::nlohmann_json
        ossrf::ossrf_nmos_api
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES CXX_EXTENSIONS NO
             POSITION_INDEPENDENT_CODE ON)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE src)

add_library(ossrf::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME})
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include" # source directory
        DESTINATION "." # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h" # select header files
)
