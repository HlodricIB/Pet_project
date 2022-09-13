cmake_minimum_required(VERSION 3.5)

project(Client_HTTP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost)
if(Boost_FOUND)
    include_directories(//usr/include/boost)
    add_executable(${PROJECT_NAME}
        ./src/main.cpp
        ./src/client_http.cpp
        ./include/client_http.h
)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC -g -O0 -Wall -Wextra)
target_link_options(${PROJECT_NAME} PUBLIC -dynamic)

set(Parser_SRC /home/nikita/C++/Pet_project/ini_and_parser/src)
set(Parser_INC /home/nikita/C++/Pet_project/ini_and_parser/include)

target_include_directories(${PROJECT_NAME} PUBLIC ./include)
include_directories(${Parser_INC})

add_library(Parser STATIC ${Parser_SRC}/parser.cpp ${Parser_INC}/parser.h)
target_link_libraries(${PROJECT_NAME} PRIVATE /usr/lib/x86_64-linux-gnu/libboost_coroutine.so.1.74.0 Parser)
