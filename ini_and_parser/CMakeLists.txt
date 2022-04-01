cmake_minimum_required(VERSION 3.5)

project(Parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.74.0)
if(Boost_FOUND)
    include_directories(//usr/include/boost/)
    add_executable(${PROJECT_NAME}
      ./src/main.cpp
      ./src/parser.cpp
  )
endif()

target_compile_options(${PROJECT_NAME} PUBLIC -g -O0 -Wall -Wextra)
target_link_options(${PROJECT_NAME} PUBLIC -dynamic)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)
