cmake_minimum_required(VERSION 3.5)

project(Logger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    ./src/main.cpp
    ./src/logger.cpp
    ./include/logger.h
)

target_compile_options(${PROJECT_NAME} PUBLIC -g -O0 -Wall -Wextra)
target_link_options(${PROJECT_NAME} PUBLIC -dynamic)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)
