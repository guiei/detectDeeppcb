QT += core gui widgets

CONFIG += c++11
TEMPLATE = app
TARGET = DefectInspect

INCLUDEPATH += $$PWD/include

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/DefectDetector.cpp \
    src/DetectionWorker.cpp \
    src/ImageUtils.cpp

HEADERS += \
    include/MainWindow.h \
    include/DefectDetector.h \
    include/DetectionWorker.h \
    include/DetectionTypes.h \
    include/ImageUtils.h \
    include/OpenCvCompat.h

unix {
    CONFIG += link_pkgconfig
    packagesExist(opencv4) {
        PKGCONFIG += opencv4
    } else:packagesExist(opencv) {
        PKGCONFIG += opencv
    } else {
        LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui
    }
}

win32 {
    # On Windows, configure OpenCV include/lib paths in Qt Creator if needed.
}
