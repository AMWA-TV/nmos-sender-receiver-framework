cmake_minimum_required(VERSION 3.16)
project(gst_nmos_video_receiver_plugin LANGUAGES CXX)

find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)

# Locate GLib package
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Locate GStreamer packages
pkg_search_module(GSTREAMER REQUIRED gstreamer-1.0>=1.4)
pkg_search_module(GSTREAMER_APP REQUIRED gstreamer-app-1.0>=1.4)
pkg_search_module(GSTREAMER_AUDIO REQUIRED gstreamer-audio-1.0>=1.4)
pkg_search_module(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.4)

# Include GLib directories and link libraries
include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})
add_definitions(${GLIB_CFLAGS_OTHER})

# Include source files for gst_nmos_video_receiver_plugin
file(GLOB_RECURSE PLUGIN_SOURCE_FILES *.cpp *.h)

# Include source files for utils
file(GLOB_RECURSE UTILS_SOURCE_FILES utils/*.cpp utils/*.h)

# Combine utils and plugin sources
set(SOURCE_FILES ${PLUGIN_SOURCE_FILES} ${UTILS_SOURCE_FILES})

# Define the plugin as a shared library
add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE ${GSTREAMER_INCLUDE_DIRS}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils/include
)

# Link GStreamer libraries
target_compile_options(${PROJECT_NAME} PRIVATE ${GSTREAMER_CFLAGS_OTHER})
target_link_libraries(${PROJECT_NAME}
    PRIVATE 
    ${GSTREAMER_LIBRARIES} 
    ${GSTREAMER_APP_LIBRARIES} 
    ${GSTREAMER_AUDIO_LIBRARIES} 
    ${GSTREAMER_VIDEO_LIBRARIES}
    PUBLIC
    bisect::project_warnings
    bisect::expected
    bisect::bisect_nmoscpp
    bisect::bisect_json
    nlohmann_json::nlohmann_json
    ossrf::ossrf_nmos_api
    ${GLIB_LIBRARIES}
)

# Specify the output directory and the library name
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    OUTPUT_NAME "gstnmosvideoreceiver"
)

add_library(ossrf::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Install the plugin to the system's GStreamer plugin directory
install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ~/.local/lib/gstreamer-1.0)
