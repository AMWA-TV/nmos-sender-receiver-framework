project(bisect_nmoscpp LANGUAGES CXX)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

find_package(fmt REQUIRED)
find_package(nmos-cpp REQUIRED)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_source_files})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE project_options project_warnings
        PUBLIC fmt::fmt
        bisect::project_warnings
        nmos-cpp::compile-settings
        nmos-cpp::nmos-cpp
        bisect::expected
        bisect::bisect_sdp
        bisect::bisect_nmoscpp
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

add_library(bisect::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME})
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include" # source directory
        DESTINATION "." # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h" # select header files
)
