cmake_minimum_required(VERSION 3.16)
project(MyBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Add debug and release specific definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG_MODE)
    message(STATUS "Building in Debug mode - extensive logging enabled")
else()
    # Don't define DEBUG_MODE for release builds
    message(STATUS "Building in Release mode - minimal logging")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineCore WebEngineWidgets)

message(STATUS "Qt6WebEngineCore_INCLUDE_DIRS: ${Qt6WebEngineCore_INCLUDE_DIRS}")
message(STATUS "Qt6WebEngineWidgets_INCLUDE_DIRS: ${Qt6WebEngineWidgets_INCLUDE_DIRS}")

add_executable(MyBrowser
    src/main.cpp

    # Feature-based organization
    # Main Window
    src/features/main-window/mainwindow.cpp
    src/features/main-window/mainwindow.h

    # WebView Features
    src/features/webview/webview.cpp
    src/features/webview/webview.h

    # Tab Widget
    src/features/tab-widget/verticaltabwidget.cpp
    src/features/tab-widget/verticaltabwidget.h

    # Command Palette
    src/features/command-palette/commandpalettedialog.cpp
    src/features/command-palette/commandpalettedialog.h
    src/features/command-palette/commandpalettemanager.cpp
    src/features/command-palette/commandpalettemanager.h

    # Workspace Management
    src/features/workspace/workspacemanager.cpp
    src/features/workspace/workspacemanager.h

    # Bookmark Management
    src/features/bookmark/bookmarkmanager.cpp
    src/features/bookmark/bookmarkmanager.h

    # Picture-in-Picture
    src/features/picture-in-picture/pictureinpicturemanager.cpp
    src/features/picture-in-picture/pictureinpicturemanager.h

    # Core
    src/core/ui_constants.h

    resources.qrc
)

# Add debug and release specific definitions to target
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(MyBrowser PRIVATE DEBUG_MODE)
else()
    # Don't define DEBUG_MODE for release builds
endif()

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
