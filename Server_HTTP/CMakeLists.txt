cmake_minimum_required(VERSION 3.5)

project(Server_HTTP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost)
if(Boost_FOUND)
    include_directories(//usr/include/boost)
    add_executable(${PROJECT_NAME}
        ./src/main.cpp
        ./src/server_http.cpp
        ./include/server_http.h
)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC -g -O0 -Wall -Wextra)
target_link_options(${PROJECT_NAME} PUBLIC -dynamic)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)
