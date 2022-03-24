cmake_minimum_required(VERSION 3.5)

add_library(DB_module STATIC ../src/DB_module.cpp ../include/DB_module.h)

find_package(PostgreSQL REQUIRED)
target_link_libraries(DB_module PRIVATE PostgreSQL::PostgreSQL)

target_include_directories(DB_module PUBLIC ../include/ ../../ini_and_parser/include/)
