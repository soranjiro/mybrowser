cmake_minimum_required(VERSION 3.16)
project(MyBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineCore WebEngineWidgets)

message(STATUS "Qt6WebEngineCore_INCLUDE_DIRS: ${Qt6WebEngineCore_INCLUDE_DIRS}")
message(STATUS "Qt6WebEngineWidgets_INCLUDE_DIRS: ${Qt6WebEngineWidgets_INCLUDE_DIRS}")

add_executable(MyBrowser
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/webview.cpp
    src/webview.h
    src/verticaltabwidget.cpp
    src/verticaltabwidget.h
    src/workspacemanager.cpp
    src/workspacemanager.h
    src/bookmarkmanager.cpp
    src/bookmarkmanager.h
    src/ui_constants.h
    resources.qrc
)

# Add this line to explicitly include directories for WebEngineCore and WebEngineWidgets
target_include_directories(MyBrowser PRIVATE
    ${Qt6WebEngineCore_INCLUDE_DIRS}
    ${Qt6WebEngineWidgets_INCLUDE_DIRS}
)

target_link_libraries(MyBrowser PRIVATE
    Qt6::Widgets
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
)
