QT += core widgets
QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

TEMPLATE = app
TARGET = tes_cpp

SOURCES += on_folder.cpp

DEFINES += QT_DEPRECATED_WARNINGS

# ====== INCLUDE PATHS ======
INCLUDEPATH += "C:/Qt/Qt5.8.0/5.8/msvc2015_64/include"
INCLUDEPATH += "C:/opencv_460/build/include"
INCLUDEPATH += "C:/boost_1_67_0"
INCLUDEPATH += "C:/json-develop/include"
INCLUDEPATH += "C:/Program Files/temp"  # Tesseract headers
INCLUDEPATH += "C:/microsoft.ml.onnxruntime.1.15.0/build/native/include"
#INCLUDEPATH += "C:/vcpkg/buildtrees/tesseract/src/5.5.1-29f78e72d6.clean/include"
#INCLUDEPATH += "C:/vcpkg/buildtrees/tesseract/src/5.5.1-29f78e72d6.clean/include"
#C:\vcpkg\buildtrees\leptonica\src\1.85.0-7512b3749c.clean\src

INCLUDEPATH += "C:/vcpkg/installed/x64-windows/include"
LIBS += -L"C:/vcpkg/installed/x64-windows/lib" -ltesseract55 -lleptonica-1.85.0
LIBS += -L"C:/vcpkg/installed/x64-windows/bin" -ltesseract55 -lleptonica-1.85.0


# ====== WINDOWS SPECIFIC CONFIGURATION ======
win32 {

    # ---- OpenCV ----
    CONFIG(debug, debug|release) {
        LIBS += -L"C:/opencv_460/build/x64/vc14/lib" \
                -lopencv_world460d
    }
    CONFIG(release, debug|release) {
        LIBS += -L"C:/opencv_460/build/x64/vc14/lib" \
                -lopencv_world460
    }

    # ---- Tesseract ----
#    LIBS += -L"C:/Program Files/temp" \
#            -ltesseract55 \
#            -lleptonica-1.85.0

    # ---- Boost ----
    CONFIG(debug, debug|release) {
        LIBS += -L"C:/boost_1_67_0/lib64-msvc-14.0" \
                -lboost_date_time-vc140-mt-gd-x64-1_67
    }
    CONFIG(release, debug|release) {
        LIBS += -L"C:/boost_1_67_0/lib64-msvc-14.0" \
                -lboost_date_time-vc140-mt-x64-1_67
    }

    # ---- ONNX Runtime ----
    LIBS += -L"C:/microsoft.ml.onnxruntime.1.15.0/runtimes/win-x64/native" \
            -lonnxruntime

    # ---- Windows system libraries ----
    LIBS += -lws2_32 -luser32 -lgdi32 -lcomdlg32 -lole32
}

# ====== UNIX / LINUX CONFIGURATION ======
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4 tesseract lept

    ONNX_PATH = /usr/local/lib/onnxruntime
    INCLUDEPATH += $$ONNX_PATH/include
    LIBS += -L$$ONNX_PATH/lib -lonnxruntime
}
