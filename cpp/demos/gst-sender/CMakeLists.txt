cmake_minimum_required(VERSION 3.16)
project(gst-sender LANGUAGES CXX)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_source_files})

target_include_directories(${PROJECT_NAME} PRIVATE ${fmt_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} 
                        PRIVATE project_options project_warnings
                        PUBLIC
                        bisect::project_warnings
                        bisect::expected
                        bisect::bisect_gst)

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

install(TARGETS ${PROJECT_NAME})
