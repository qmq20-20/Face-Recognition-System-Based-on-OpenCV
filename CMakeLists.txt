cmake_minimum_required(VERSION 3.16)

project(FaceRecognitionSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

# Default local dependency paths. Override them from the command line when needed:
# cmake -S . -B build -DCMAKE_PREFIX_PATH=... -DOpenCV_DIR=...
if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "D:/QT/5.14.2/mingw73_64" CACHE PATH "Qt installation prefix")
endif()

if(NOT OpenCV_DIR)
    set(OpenCV_DIR "D:/opencv-mingw/x64/mingw/lib" CACHE PATH "OpenCV CMake package directory")
endif()

find_package(Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(OpenCV REQUIRED)

add_executable(FaceRecognitionSystem
    src/main.cpp
    src/config/AppConfig.h
    src/config/AppConfig.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PersonManagementDialog.h
    src/ui/PersonManagementDialog.cpp
    src/ui/RegisterDialog.h
    src/ui/RegisterDialog.cpp
    src/vision/FaceDetector.h
    src/vision/FaceDetector.cpp
    src/vision/FeatureExtractor.h
    src/vision/FeatureExtractor.cpp
    src/storage/FaceRepository.h
    src/storage/FaceRepository.cpp
    src/service/RecognitionService.h
    src/service/RecognitionService.cpp
    src/utils/ImageUtils.h
    src/utils/ImageUtils.cpp
)
add_custom_command(TARGET FaceRecognitionSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:FaceRecognitionSystem>/resources
)

target_include_directories(FaceRecognitionSystem
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(FaceRecognitionSystem
    PRIVATE
    Qt5::Widgets
    Qt5::Sql
    ${OpenCV_LIBS}
)
