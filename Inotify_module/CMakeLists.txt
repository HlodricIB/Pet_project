cmake_minimum_required(VERSION 3.5)

project(Inotify_module LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    ./src/main.cpp
    ./src/inotify_module.cpp
    ./include/inotify_module.h
)

target_compile_options(${PROJECT_NAME} PUBLIC -g -O0 -Wall -Wextra)
target_link_options(${PROJECT_NAME} PUBLIC -dynamic)

set(Logger_SRC /home/nikita/C++/Pet_project/Logger/src)
set(Logger_INC /home/nikita/C++/Pet_project/Logger/include)
set(Parser_SRC /home/nikita/C++/Pet_project/ini_and_parser/src)
set(Parser_INC /home/nikita/C++/Pet_project/ini_and_parser/include)
set(DB_module_SRC /home/nikita/C++/Pet_project/DB_module/src)
set(DB_module_INC /home/nikita/C++/Pet_project/DB_module/include)
set(Handler_SRC /home/nikita/C++/Pet_project/Handler/src)
set(Handler_INC /home/nikita/C++/Pet_project/Handler/include)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)
include_directories(${Logger_INC} ${Parser_INC} ${DB_module_INC} ${Handler_INC})

add_library(Logger STATIC ${Logger_SRC}/logger.cpp ${Logger_INC}/logger.h)
add_library(Parser STATIC ${Parser_SRC}/parser.cpp ${Parser_INC}/parser.h)
add_library(DB_module STATIC ${DB_module_SRC}/DB_module.cpp ${DB_module_INC}/DB_module.h)
add_library(Handler STATIC ${Handler_SRC}/handler.cpp ${Handler_INC}/handler.h)
target_link_libraries(DB_module PRIVATE PostgreSQL::PostgreSQL)
target_link_libraries(Handler PRIVATE PostgreSQL::PostgreSQL)
find_package(PostgreSQL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE PostgreSQL::PostgreSQL Logger Parser DB_module Handler)
