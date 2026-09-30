cmake_minimum_required(VERSION 3.16)
project(FallDetectionAdmin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Charts Multimedia)

set(SRC_FILES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/safezoneoverlay.cpp
    src/safezoneoverlay.h
    src/safezonedialog.cpp
    src/safezonedialog.h
)

add_executable(FallDetectionAdmin ${SRC_FILES})

target_link_libraries(FallDetectionAdmin PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts
    Qt6::Multimedia
)