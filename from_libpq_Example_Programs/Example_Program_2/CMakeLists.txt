cmake_minimum_required(VERSION 3.5)

project(Program_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    ./src/main.cpp
)

find_package(PostgreSQL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE PostgreSQL::PostgreSQL)

target_compile_options(${PROJECT_NAME} PUBLIC -g -O0 -Wall -Wextra)
target_link_options(${PROJECT_NAME} PUBLIC -dynamic)
