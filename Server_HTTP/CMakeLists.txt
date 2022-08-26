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

set(Parser_SRC /home/nikita/C++/Pet_project/ini_and_parser/src)
set(Parser_INC /home/nikita/C++/Pet_project/ini_and_parser/include)
set(Logger_SRC /home/nikita/C++/Pet_project/Logger/src)
set(Logger_INC /home/nikita/C++/Pet_project/Logger/include)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)
include_directories(${Parser_INC} ${Logger_INC})

add_library(Parser STATIC ${Parser_SRC}/parser.cpp ${Parser_INC}/parser.h)
add_library(Logger STATIC ${Logger_SRC}/logger.cpp ${Logger_INC}/logger.h)
target_link_libraries(${PROJECT_NAME} PRIVATE Parser Logger)
