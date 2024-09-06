project(ossrf_gstreamer_api LANGUAGES CXX)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_source_files})

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE project_options project_warnings
        PUBLIC
        bisect::project_warnings
        bisect::expected
        bisect::bisect_gst
        bisect::bisect_json
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES CXX_EXTENSIONS NO
             POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
pkg_search_module(gstreamer-audio REQUIRED IMPORTED_TARGET gstreamer-audio-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC 
        PkgConfig::gstreamer
        PkgConfig::gstreamer-app
        PkgConfig::gstreamer-audio
        PkgConfig::gstreamer-video
)

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