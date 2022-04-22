cmake_minimum_required(VERSION 3.5)

project(DB_module LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    ./src/main.cpp
    ./src/DB_module.cpp
    ./include/DB_module.h
)

find_package(PostgreSQL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE PostgreSQL::PostgreSQL)

target_compile_options(${PROJECT_NAME} PUBLIC -g -O0 -Wall -Wextra)
target_link_options(${PROJECT_NAME} PUBLIC -dynamic)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/ /home/nikita/C++/Pet_project/ini_and_parser/include/)
