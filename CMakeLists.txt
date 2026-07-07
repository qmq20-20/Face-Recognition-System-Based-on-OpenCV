cmake_minimum_required(VERSION 3.16)

project(FaceRecognitionSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

# Qt path
set(CMAKE_PREFIX_PATH "D:/QT/5.14.2/mingw73_64")

# OpenCV path
set(OpenCV_DIR "D:/opencv-mingw/x64/mingw/lib")

find_package(Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

add_executable(FaceRecognitionSystem
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/FaceDetector.h
    src/FaceDetector.cpp
    src/FeatureExtractor.h
    src/FeatureExtractor.cpp
    src/FaceRepository.h
    src/FaceRepository.cpp
    src/RegisterDialog.h
    src/RegisterDialog.cpp
)
add_custom_command(TARGET FaceRecognitionSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:FaceRecognitionSystem>/resources
)
target_link_libraries(FaceRecognitionSystem
    PRIVATE
    Qt5::Widgets
    Qt5::Sql
    ${OpenCV_LIBS}
)