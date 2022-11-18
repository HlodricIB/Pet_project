cmake_minimum_required(VERSION 3.5)

project(Parser_unit_tests LANGUAGES CXX)

add_definitions(-DGTEST_LANGUAGE_CXX17)

find_package(Threads REQUIRED)

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
set(Parser_SRC /home/nikita/C++/Pet_project/ini_and_parser/src)
set(Parser_INC /home/nikita/C++/Pet_project/ini_and_parser/include)
if (NOT ${GMockSrc} STREQUAL "")
    list(APPEND GTestFiles ${GMockSrc}/src/gmock-all.cc)
    list(APPEND GTestIncludes ${GMockSrc} ${GMockSrc}/include)
endif ()

include_directories(${GTestIncludes} ${Parser_INC})

add_executable(${PROJECT_NAME}
    ./src/main.cpp
    ./src/tst_parser_unit_tests.cpp
    ./include/tst_parser_unit_tests.h
    ${GTestFiles}
)

target_include_directories(${PROJECT_NAME} PUBLIC ./include/)

add_test(NAME Parser_unit_tests COMMAND Parser_unit_tests)
add_library(Parser STATIC ${Parser_SRC}/parser.cpp ${Parser_INC}/parser.h)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads Parser)

