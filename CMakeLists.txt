cmake_minimum_required(VERSION 3.16)

project(FaceRecognitionSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

set(CMAKE_PREFIX_PATH "D:/QT/5.14.2/mingw73_64")

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(FaceRecognitionSystem
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(FaceRecognitionSystem PRIVATE Qt5::Widgets)