cmake_minimum_required(VERSION 3.16)
project(ossrf-nmos-api LANGUAGES CXX)

find_package(nlohmann_json REQUIRED)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_source_files})

target_include_directories(${PROJECT_NAME} PRIVATE ${fmt_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} 
                        PRIVATE project_options project_warnings
                        nlohmann_json::nlohmann_json
                        PUBLIC
                        bisect::project_warnings
                        bisect::expected
                        bisect::bisect_json
                        ossrf::ossrf_nmos_api
                        ossrf::ossrf_gstreamer_api)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

install(TARGETS ${PROJECT_NAME})
