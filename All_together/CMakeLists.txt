cmake_minimum_required(VERSION 3.5)

project(Pet_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    ./src/main.cpp
)

add_subdirectory(../ini_and_parser/As_library ../ini_and_parser/As_library/bin)

target_link_libraries(${PROJECT_NAME} Parser)

target_include_directories(${PROJECT_NAME} PUBLIC ../ini_and_parser/include/)
