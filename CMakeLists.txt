cmake_minimum_required(VERSION 3.5)

project(Demo_combined LANGUAGES CXX VERSION 0.1.1.0)

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
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets SerialPort Network REQUIRED)

if (WIN32)
	include_directories(${Hik_SDK_DIR}/include/win10_x64)
	include_directories(${HQV_DIR}/include)
	include_directories(${EURESYS_DIR}/include)
	include_directories(${OpenCV_DIR}/include)

	link_directories(${Hik_SDK_DIR}/lib/win64_x64)
	link_directories(${HQV_DIR}/lib)
	link_directories(${EURESYS_DIR}/lib)
	link_directories(${OpenCV_DIR}/lib/win64_x64)
elseif (APPLE)

# for aarch64 only
elseif (UNIX)
	include_directories(${Hik_SDK_DIR}/include/aarch64)
	include_directories(${HQV_DIR}/include)
	include_directories(${EURESYS_DIR}/include)
	include_directories(${OpenCV_DIR}/include)

	link_directories(${Hik_SDK_DIR}/lib/aarch64)
	link_directories(${HQV_DIR}/lib)
	link_directories(${EURESYS_DIR}/lib)
	link_directories(${OpenCV_DIR}/lib/aarch64)
endif()

if (MSVC)
    set(VER_PRELEASE 1)
    configure_file("${CMAKE_SOURCE_DIR}/src/gist.rc.in" "${CMAKE_SOURCE_DIR}/resources/gist.rc" NEWLINE_STYLE WIN32)
endif()

# for now
set(SOURCE_FILES
    src/main.cpp
    src/utils.h
    src/utils.cpp
    src/demo.h
    src/demo.cpp
    src/threadpool.h
    src/threadpool.cpp
    src/mywidget.h
    src/mywidget.cpp
    src/progsettings.h
    src/progsettings.cpp
    src/imageproc.h
    src/imageproc.cpp
#    src/cam.h
#    src/cam.cpp
    src/mvcam.h
    src/mvcam.cpp
#    src/hqvcam.h
#    src/hqvcam.cpp
#    src/euresyscam.h
#    src/euresyscam.cpp
    src/joystick.h
    src/joystick.cpp
    src/demo_dev.ui
    src/settings.ui
    resources/resources.qrc
    resources/gist.rc
#    preferences.h
#    preferences.cpp
#    preferences.ui
)

set(PROJECT_LIBS
    Qt${QT_VERSION_MAJOR}::Widgets
    # mvs
    MvCameraControl
    # HQV
#    HQV_AVIProcess64
#    HQV_DataConvert64
#    HQV_SDK64
    # Euresys
#    MultiCam
#    clallserial

)
if (WIN32)
    set(PROJECT_LIBS
        ${PROJECT_LIBS}
        # joystick
        winmm
        # opencv
        opencv_core454
        opencv_imgcodecs454
        opencv_imgproc454
        opencv_videoio454
    )
elseif (APPLE)

# for aarch64 only
elseif (UNIX)
    set(PROJECT_LIBS
        ${PROJECT_LIBS}
        # opencv
        opencv_core
        opencv_imgcodecs
        opencv_imgproc
        opencv_videoio
        # pthread
        pthread
    )
endif()

add_executable(Demo_param WIN32 ${TRANSLATE} ${SOURCE_FILES})
target_compile_definitions(Demo_param PRIVATE PARAM=1)
qt5_use_modules(Demo_param Widgets SerialPort Network)
target_link_libraries(Demo_param ${PROJECT_LIBS})

add_executable(Demo_ICMOS WIN32 ${TRANSLATE} ${SOURCE_FILES})
target_compile_definitions(Demo_ICMOS PRIVATE ICMOS=1)
qt5_use_modules(Demo_ICMOS Widgets SerialPort Network)
target_link_libraries(Demo_ICMOS ${PROJECT_LIBS})
