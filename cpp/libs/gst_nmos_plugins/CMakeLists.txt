cmake_minimum_required(VERSION 3.16)
project(gst_nmos_plugins LANGUAGES CXX)

find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)

# Locate GLib package
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Locate GStreamer packages
pkg_search_module(GSTREAMER REQUIRED gstreamer-1.0>=1.4)
pkg_search_module(GSTREAMER_APP REQUIRED gstreamer-app-1.0>=1.4)
pkg_search_module(GSTREAMER_AUDIO REQUIRED gstreamer-audio-1.0>=1.4)
pkg_search_module(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.4)

# Include the parent directory of gst_nmos_plugins
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Utils library (shared across plugins)
add_library(utils STATIC src/utils.cpp)

# Enable -fPIC for utils
set_target_properties(utils PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(utils
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE ${GLIB_INCLUDE_DIRS}
    PRIVATE ${GSTREAMER_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../bisect
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ossrf
)
target_link_libraries(utils
    PUBLIC nlohmann_json::nlohmann_json
    PRIVATE ${GLIB_LIBRARIES}
    PRIVATE ${GSTREAMER_LIBRARIES}
    PRIVATE ${GSTREAMER_APP_LIBRARIES}
    PRIVATE ${GSTREAMER_AUDIO_LIBRARIES}
    PRIVATE ${GSTREAMER_VIDEO_LIBRARIES}
    PRIVATE bisect::project_warnings
    PRIVATE bisect::expected
    PRIVATE bisect::bisect_nmoscpp
    PRIVATE bisect::bisect_json
    PRIVATE ossrf::ossrf_nmos_api
)

# Function to create a plugin target
function(create_plugin plugin_name plugin_sources output_name)
    add_library(${plugin_name} MODULE ${plugin_sources})
    target_include_directories(${plugin_name}
        PRIVATE ${GSTREAMER_INCLUDE_DIRS}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
    target_link_libraries(${plugin_name}
        PRIVATE
        ${GSTREAMER_LIBRARIES}
        ${GSTREAMER_APP_LIBRARIES}
        ${GSTREAMER_AUDIO_LIBRARIES}
        ${GSTREAMER_VIDEO_LIBRARIES}
        utils
        PUBLIC
        bisect::project_warnings
        bisect::expected
        bisect::bisect_nmoscpp
        bisect::bisect_json
        nlohmann_json::nlohmann_json
        ossrf::ossrf_nmos_api
        ${GLIB_LIBRARIES}
    )
    set_target_properties(${plugin_name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
        OUTPUT_NAME ${output_name}
    )
    add_library(ossrf::${plugin_name} ALIAS ${plugin_name})
    install(TARGETS ${plugin_name}
        LIBRARY DESTINATION ~/.local/lib/gstreamer-1.0
    )
endfunction()

# Plugin: Video Receiver
create_plugin(
    gst_nmos_video_receiver_plugin
    src/gst_nmos_video_receiver_plugin.cpp
    "gstnmosvideoreceiver"
)

# Plugin: Audio Receiver
create_plugin(
    gst_nmos_audio_receiver_plugin
    src/gst_nmos_audio_receiver_plugin.cpp
    "gstnmosaudioreceiver"
)

# Plugin: Sender
create_plugin(
    gst_nmos_sender_plugin
    src/gst_nmos_sender_plugin.cpp
    "gstnmossender"
)
