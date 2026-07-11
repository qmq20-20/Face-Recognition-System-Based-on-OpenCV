cmake_minimum_required(VERSION 3.16)

project(FaceRecognitionSystem LANGUAGES CXX)

option(BUILD_YOLO_MODEL_PROBE "Build the local YOLO ONNX compatibility probe" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

# Default local dependency paths. Override them from the command line when needed:
# cmake -S . -B build -DCMAKE_PREFIX_PATH=... -DOpenCV_DIR=...
if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "D:/QT/5.14.2/mingw73_64" CACHE PATH "Qt installation prefix")
endif()

if(NOT OpenCV_DIR)
    set(LOCAL_OPENCV5_DIR "${CMAKE_SOURCE_DIR}/opencv5-deps/install/x64/mingw/lib")

    if(EXISTS "${LOCAL_OPENCV5_DIR}/OpenCVConfig.cmake")
        set(OpenCV_DIR "${LOCAL_OPENCV5_DIR}" CACHE PATH "OpenCV CMake package directory")
    else()
        set(OpenCV_DIR "D:/opencv-mingw/x64/mingw/lib" CACHE PATH "OpenCV CMake package directory")
    endif()
endif()

find_package(Qt5 REQUIRED COMPONENTS Widgets Sql Concurrent)
find_package(OpenCV REQUIRED)

if(OpenCV_VERSION VERSION_LESS "5.0.0")
    message(WARNING
        "OpenCV ${OpenCV_VERSION} can run the Haar fallback, but its DNN module is not compatible with the project's YOLO11 ONNX model. "
        "Use an OpenCV 5.0.0 or newer MinGW build to enable YOLO detection.")
endif()

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
    src/vision/FaceDetection.h
    src/vision/FeatureExtractor.h
    src/vision/FeatureExtractor.cpp
    src/storage/FaceRepository.h
    src/storage/FaceRepository.cpp
    src/service/RecognitionService.h
    src/service/RecognitionService.cpp
    src/service/CameraRecognitionResult.h
    src/utils/ImageUtils.h
    src/utils/ImageUtils.cpp
)
add_custom_command(TARGET FaceRecognitionSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:FaceRecognitionSystem>/resources
)

get_filename_component(OPENCV_BIN_DIR "${OpenCV_DIR}/../bin" ABSOLUTE)
get_filename_component(MINGW_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

function(copy_opencv_runtime target_name)
    if(EXISTS "${OPENCV_BIN_DIR}")
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${OPENCV_BIN_DIR}
            $<TARGET_FILE_DIR:${target_name}>
        )
    endif()
endfunction()

function(copy_mingw_runtime target_name)
    foreach(runtime_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        if(EXISTS "${MINGW_RUNTIME_DIR}/${runtime_dll}")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_RUNTIME_DIR}/${runtime_dll}"
                $<TARGET_FILE_DIR:${target_name}>
            )
        endif()
    endforeach()
endfunction()

copy_opencv_runtime(FaceRecognitionSystem)

find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt.exe windeployqt
    HINTS "${CMAKE_PREFIX_PATH}/bin"
)

if(WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET FaceRecognitionSystem POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --force $<TARGET_FILE:FaceRecognitionSystem>
    )
endif()

copy_mingw_runtime(FaceRecognitionSystem)

target_include_directories(FaceRecognitionSystem
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(FaceRecognitionSystem
    PRIVATE
    Qt5::Widgets
    Qt5::Sql
    Qt5::Concurrent
    ${OpenCV_LIBS}
)

if(BUILD_YOLO_MODEL_PROBE)
    add_executable(YoloModelProbe
        tools/YoloModelProbe.cpp
        src/config/AppConfig.h
        src/config/AppConfig.cpp
        src/vision/FaceDetection.h
        src/vision/FaceDetector.h
        src/vision/FaceDetector.cpp
    )

    target_include_directories(YoloModelProbe
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(YoloModelProbe
        PRIVATE
        Qt5::Core
        ${OpenCV_LIBS}
    )

    copy_opencv_runtime(YoloModelProbe)
    copy_mingw_runtime(YoloModelProbe)
endif()
