cmake_minimum_required(VERSION 3.5)

project(Demo_combined LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDK_DIR "${PROJECT_SOURCE_DIR}/sdk64")

set(Hik_SDK_DIR "${SDK_DIR}/mvs")
set(HQV_DIR "${SDK_DIR}/HQV")
set(EURESYS_DIR "${SDK_DIR}/euresys")
set(OpenCV_DIR "${SDK_DIR}/opencv")

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
include_directories(${EURESYS_DIR}/include)
include_directories(${OpenCV_DIR}/include)

link_directories(${Hik_SDK_DIR}/lib)
link_directories(${HQV_DIR}/lib)
link_directories(${EURESYS_DIR}/lib)
link_directories(${OpenCV_DIR}/lib)

set(SOURCE_FILES
	main.cpp
	utils.cpp
	demo.cpp
	threadpool.cpp
	mywidget.cpp
	progsettings.cpp
	imageproc.cpp
	cam.cpp
#	mvcam.cpp
#	hqvcam.cpp
#	euresyscam.cpp
    utils.h
	demo.h
	threadpool.h
	mywidget.h
	progsettings.h
	imageproc.h
	cam.h
#	mvcam.h
#	hqvcam.h
#	euresyscam.h
    demo_dev.ui
	settings.ui
	resources/resources.qrc
	icon.rc
)

set(PROJECT_LIBS
	Qt${QT_VERSION_MAJOR}::Widgets
	# mvs
	MvCameraControl
	# HQV
	HQV_AVIProcess64
	HQV_DataConvert64
	HQV_SDK64
	# Euresys
#	MultiCam
#	clallserial
    # opencv
	opencv_core454
	opencv_imgcodecs454
	opencv_imgproc454
	opencv_videoio454
)

add_executable(Demo_param WIN32 ${SOURCE_FILES})
target_compile_definitions(Demo_param PRIVATE PARAM=1)
qt5_use_modules(Demo_param Widgets SerialPort)
target_link_libraries(Demo_param ${PROJECT_LIBS})

add_executable(Demo_ICMOS WIN32 ${SOURCE_FILES})
target_compile_definitions(Demo_ICMOS PRIVATE ICMOS=1)
qt5_use_modules(Demo_ICMOS Widgets SerialPort)
target_link_libraries(Demo_ICMOS ${PROJECT_LIBS})
