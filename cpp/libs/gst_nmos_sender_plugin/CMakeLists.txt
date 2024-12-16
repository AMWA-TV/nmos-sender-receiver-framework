cmake_minimum_required(VERSION 3.16)
project(gst_nmos_sender_plugin LANGUAGES C)

# Define the path to the ossrf_c_nmos_api directory
set(OSSRF_C_NMOS_API_PATH "../ossrf_c_nmos_api/")

# Find required GStreamer packages
find_package(PkgConfig REQUIRED)
pkg_search_module(GSTREAMER REQUIRED gstreamer-1.0>=1.4)
pkg_search_module(GSTREAMER_APP REQUIRED gstreamer-app-1.0>=1.4)
pkg_search_module(GSTREAMER_AUDIO REQUIRED gstreamer-audio-1.0>=1.4)
pkg_search_module(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.4)

# Include source files
file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.c *.h)

# Define the plugin as a shared library
add_library(${PROJECT_NAME} MODULE ${${PROJECT_NAME}_source_files})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${GSTREAMER_INCLUDE_DIRS} ${OSSRF_C_NMOS_API_PATH})

# Link GStreamer libraries
target_compile_options(${PROJECT_NAME} PRIVATE ${GSTREAMER_CFLAGS_OTHER})
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${GSTREAMER_LIBRARIES} 
    ${GSTREAMER_APP_LIBRARIES} 
    ${GSTREAMER_AUDIO_LIBRARIES} 
    ${GSTREAMER_VIDEO_LIBRARIES}
    ossrf::ossrf_c_nmos_api)

# Specify the output directory and the library name
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    OUTPUT_NAME "gstnmossender"  # Custom .so name
)

add_library(ossrf::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Install the plugin to the system's GStreamer plugin directory
install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ~/.local/lib/gstreamer-1.0)
