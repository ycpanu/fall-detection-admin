cmake_minimum_required(VERSION 3.16)
project(FallDetectionAdmin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)   #自动处理 Q_OBJECT 宏
set(CMAKE_AUTORCC ON)   #自动编译 .qrc 资源文件
set(CMAKE_AUTOUIC ON)   #自动把 .ui 文件转成 C++ 代码

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Charts)

# 收集源码文件
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

set(SRC_FILES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
)

# 创建可执行文件
add_executable(FallDetectionAdmin ${SRC_FILES})

# 链接 Qt 的核心动态库
target_link_libraries(FallDetectionAdmin PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts
)
