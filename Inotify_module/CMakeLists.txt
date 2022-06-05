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

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)
include_directories(${Logger_INC}/)

add_library(Logger STATIC ${Logger_SRC}/logger.cpp ${Logger_INC}/logger.h)
target_link_libraries(${PROJECT_NAME} PRIVATE Logger)
