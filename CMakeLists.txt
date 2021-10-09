cmake_minimum_required(VERSION 3.5)

project(Demo_combined LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Hik_SDK_DIR "${PROJECT_SOURCE_DIR}/sdk32/mvs")
set(HQV_DIR "${PROJECT_SOURCE_DIR}/sdk32/HQV")
set(OpenCV_DIR "${PROJECT_SOURCE_DIR}/sdk32/opencv")

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt5 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets SerialPort REQUIRED)

include_directories(${Hik_SDK_DIR}/include)
include_directories(${HQV_DIR}/include)
include_directories(${OpenCV_DIR}/include)

link_directories(${Hik_SDK_DIR}/lib)
link_directories(${HQV_DIR}/lib)
link_directories(${OpenCV_DIR}/lib)

add_executable(Demo_param WIN32
    main.cpp
    demo.cpp
    mywidget.cpp
    imageproc.cpp
	mvcam.cpp
    demo.h
    mywidget.h
    imageproc.h
	mvcam.h
    demo_dev.ui
    resources/resources.qrc
    icon.rc
    zh.qm
)
target_compile_definitions(Demo_param PRIVATE PARAM=1)

qt5_use_modules(Demo_param Widgets SerialPort)

target_link_libraries(Demo_param
    Qt${QT_VERSION_MAJOR}::Widgets
    # mvs
	MvCameraControl
    # HQV
	HQV_AVIProcess
	HQV_DataConvert
	HQV_SDK
    # opencv
	libopencv_core452
	libopencv_imgcodecs452
	libopencv_imgproc452
	libopencv_videoio452
)

add_executable(Demo_release WIN32
    main.cpp
    demo.cpp
    mywidget.cpp
    imageproc.cpp
    cam.cpp
    demo.h
    mywidget.h
    imageproc.h
    cam.h
    demo_rls.ui
    resources/resources.qrc
    icon.rc
    zh.qm
)

qt5_use_modules(Demo_release Widgets SerialPort)

target_link_libraries(Demo_release
    Qt${QT_VERSION_MAJOR}::Widgets
    # mvs
    MvCameraControl
    # HQV
    HQV_AVIProcess
    HQV_DataConvert
    HQV_SDK
    # opencv
    libopencv_core452
    libopencv_imgcodecs452
    libopencv_imgproc452
    libopencv_videoio452
)
