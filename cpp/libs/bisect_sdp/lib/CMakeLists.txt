project(bisect_sdp LANGUAGES CXX)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

find_package(nmos-cpp REQUIRED)
find_package(fmt REQUIRED)


add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_source_files})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE 
        bisect::project_options bisect::project_warnings
        PUBLIC 
        bisect::bisect_json
        bisect::bisect_nmoscpp
        bisect::expected
        nmos-cpp::compile-settings
        nmos-cpp::nmos-cpp
        fmt::fmt
        )

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES CXX_EXTENSIONS NO
             POSITION_INDEPENDENT_CODE ON)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE lib/src)

add_library(bisect::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME})
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include" # source directory
        DESTINATION "." # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h" # select header files
)
