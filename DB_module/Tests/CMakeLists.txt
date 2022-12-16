cmake_minimum_required(VERSION 3.5)

project(DB_module_unit_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DGTEST_LANGUAGE_CXX17)

find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)

if ($ENV{GOOGLETEST_DIR})
    set(GOOGLETEST_DIR $ENV{GOOGLETEST_DIR})
else ()
    if (NOT "" STREQUAL "")
        message(WARNING "Using googletest src dir specified at Qt Creator wizard")
    endif ()
    set(GOOGLETEST_DIR "")
endif ()
if (EXISTS ${GOOGLETEST_DIR})
    set(GTestSrc ${GOOGLETEST_DIR}/googletest)
    set(GMockSrc ${GOOGLETEST_DIR}/googlemock)
elseif (UNIX AND EXISTS /usr/src/gtest)
    set(GTestSrc /usr/src/gtest)
    message(WARNING "Using gtest from system")
    if (EXISTS /usr/src/gmock)
        set(GMockSrc /usr/src/gmock)
    endif ()
else ()
    message( FATAL_ERROR "No googletest src dir found - set GOOGLETEST_DIR to enable!")
endif ()

set(GTestFiles ${GTestSrc}/src/gtest-all.cc)
set(GTestIncludes ${GTestSrc} ${GTestSrc}/include)
set(Parser_SRC ../../ini_and_parser/src)
set(Parser_INC ../../ini_and_parser/include)
set(DB_module_SRC ../src)
set(DB_module_INC ../include)
if (NOT ${GMockSrc} STREQUAL "")
    list(APPEND GTestFiles ${GMockSrc}/src/gmock-all.cc)
    list(APPEND GTestIncludes ${GMockSrc} ${GMockSrc}/include)
endif ()

include_directories(${GTestIncludes} ${DB_module_INC} ${Parser_INC})

add_executable(${PROJECT_NAME}
    ./src/main.cpp
    ./src/tst_db_module_unit_tests.cpp
    ./include/tst_db_module_unit_tests.h
    ${GTestFiles}
)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
add_library(DB_module STATIC ${DB_module_SRC}/DB_module.cpp ${DB_module_INC}/DB_module.h)
add_library(Parser STATIC ${Parser_SRC}/main.cpp ${Parser_SRC}/parser.cpp ${Parser_INC}/parser.h)
target_link_libraries(DB_module PRIVATE PostgreSQL::PostgreSQL)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads PostgreSQL::PostgreSQL DB_module Parser)

