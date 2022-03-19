cmake_minimum_required(VERSION 3.5)

add_library(Parser STATIC ../src/parser.cpp ../include/parser.h)

target_include_directories(Parser PUBLIC ../include/)
