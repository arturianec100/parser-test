cmake_minimum_required(VERSION 3.5)

project(ParserTest LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

#add_compile_definitions(USE_SPIRIT_PARSER)

list(APPEND SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/parseresult.h
    src/parser.hpp
    src/hwparser.h
    src/hwparser.cpp
)

if(DEFINED USE_SPIRIT_PARSER)
    list(APPEND SOURCES
        src/spiritparser.hpp
    )
endif()

add_executable(ParserTest ${SOURCES})

target_link_libraries(ParserTest PRIVATE Qt5::Widgets)
