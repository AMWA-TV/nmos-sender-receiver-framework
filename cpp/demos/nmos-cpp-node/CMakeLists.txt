cmake_minimum_required(VERSION 3.16)
project(nmos-cpp-node LANGUAGES CXX)

file(GLOB_RECURSE ${PROJECT_NAME}_source_files *.cpp *.h)

find_package(nmos-cpp REQUIRED)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_source_files})

target_include_directories(${PROJECT_NAME} PRIVATE ${fmt_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} 
                      nmos-cpp::compile-settings
                      nmos-cpp::nmos-cpp)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

install(TARGETS ${PROJECT_NAME})
