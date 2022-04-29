cmake_minimum_required(VERSION 3.5)

project(DB_module_unit_tests LANGUAGES CXX)

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
if (NOT ${GMockSrc} STREQUAL "")
    list(APPEND GTestFiles ${GMockSrc}/src/gmock-all.cc)
    list(APPEND GTestIncludes ${GMockSrc} ${GMockSrc}/include)
endif ()

include_directories(${GTestIncludes} ../include /home/nikita/C++/Pet_project/ini_and_parser/include/)

add_executable(DB_module_unit_tests main.cpp tst_db_module_unit_tests.cpp tst_db_module_unit_tests.h
               ${GTestFiles})
add_test(NAME DB_module_unit_tests COMMAND DB_module_unit_tests)
add_library(DB_module STATIC ../src/DB_module.cpp ../include/DB_module.h)
target_link_libraries(DB_module PRIVATE PostgreSQL::PostgreSQL)
target_link_libraries(DB_module_unit_tests PRIVATE Threads::Threads PostgreSQL::PostgreSQL DB_module)

