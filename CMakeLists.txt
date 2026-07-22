cmake_minimum_required(VERSION 2.8.12)

project(DefectInspect CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
# CentOS 7's OpenCV 2.4 package may provide opencv.pc without
# installing OpenCVConfig.cmake. Prefer the CMake package and fall back to
# pkg-config so both old CentOS packages and newer OpenCV versions work.
find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OPENCV REQUIRED opencv)
    set(OpenCV_INCLUDE_DIRS ${OPENCV_INCLUDE_DIRS})
    set(OpenCV_LIBS ${OPENCV_LIBRARIES})
    link_directories(${OPENCV_LIBRARY_DIRS})
    add_definitions(${OPENCV_CFLAGS_OTHER})
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

add_executable(DefectInspect
    src/main.cpp
    src/MainWindow.cpp
    src/DefectDetector.cpp
    src/DetectionWorker.cpp
    src/ImageUtils.cpp
    include/MainWindow.h
    include/DefectDetector.h
    include/DetectionWorker.h
    include/DetectionTypes.h
    include/ImageUtils.h
    include/OpenCvCompat.h
)

target_link_libraries(DefectInspect
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    ${OpenCV_LIBS}
)
