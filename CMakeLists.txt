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
set(OpenCV_DIR "${SDK_DIR}/opencv_static")

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

include_directories(${Hik_SDK_DIR}/include)
include_directories(${HQV_DIR}/include)
include_directories(${EURESYS_DIR}/include)
include_directories(${OpenCV_DIR}/include)

link_directories(${Hik_SDK_DIR}/lib)
link_directories(${HQV_DIR}/lib)
link_directories(${EURESYS_DIR}/lib)
link_directories(${OpenCV_DIR}/lib/x64)

set(SOURCE_FILES
	main.cpp
	utils.cpp
	demo.cpp
	threadpool.cpp
	mywidget.cpp
	progsettings.cpp
	imageproc.cpp
#	cam.cpp
    mvcam.cpp
#	hqvcam.cpp
#	euresyscam.cpp
    utils.h
	demo.h
	threadpool.h
	mywidget.h
	progsettings.h
	imageproc.h
#	cam.h
    mvcam.h
#	hqvcam.h
#	euresyscam.h
    demo_dev.ui
	settings.ui
	resources/resources.qrc
	icon.rc
)

string(LENGTH ${QT_DIR} LEN_QT_DIR)
math(EXPR LEN_QT_DIR ${LEN_QT_DIR}-14)
string(SUBSTRING ${QT_DIR} 0 ${LEN_QT_DIR} QT_ROOT_DIR)
link_directories(${QT_ROOT_DIR}/lib)
link_directories(${QT_ROOT_DIR}/plugins/platforms)

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
    # windows static runtime lib
    setupapi      #QtSerialPort qserialportinfo_win.obj
    ws2_32        #Qt5Network qhostaddress.obj qhostinfo_win.obj qnativesocketengine.obj qnativesocketengine_win.obj
    iphlpapi      #Qt5Network qnetworkinterface_win.obj
    qtpcre2       #Qt5Core qregularexpression.obj
    netapi32      #Qt5Core qfilesystemengine_win.obj
    userenv       #Qt5Core qfilesystemengine_win.obj
    mincore       #Qt5Core qcoreapplication.obj
    Winmm         #Qt5Core qeventdispatcher_win.obj
    qtharfbuzz    #Qt5Gui qtextengine.obj
    qtlibpng      #QtGui qpnghandler.obj
#            opencv_world454
    libopenjp2
    libjpeg-turbo
    ittnotify
    IlmImf
    libwebp
    libtiff
    libprotobuf
    libpng
    zlib
    ippiw
    ippicvmt
    ade
    quirc
#            ippicvmt      #opencv_core454
#            ippiw         #opencv_core454
#            ittnotify     #opencv_core454
#            zlib          #opencv_core454
#            libopenjp2    #opencv_imgcodecs454
#            libjpeg-turbo #opencv_imgcodecs454
#            libtiff       #opencv_imgcodecs454
#            libwebp       #opencv_imgcodecs454
#            IlmImf        #opencv_imgcodecs454
    qwindows      #platform plugin
    Dwmapi        #qwindowswindow
    Wtsapi32      #qwindowscontext
    Imm32         #qwindowsinputcontext
    Qt5ThemeSupport
#            Qt5PrintSupport
    Qt5FontDatabaseSupport
    qtfreetype
#            Qt5FbSupport
#            Qt5PlatformCompositorSupport
#            Qt5EglSupport
    Qt5EventDispatcherSupport
#            Qt5DeviceDiscoverySupport
    Qt5WindowsUIAutomationSupport
#            Qt5AccessibilitySupport

)

set(CompilerFlags
    CMAKE_C_FLAGS
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()
add_definitions(-DBUILD_STATIC=1)

add_executable(Demo_param WIN32 ${SOURCE_FILES})
target_compile_definitions(Demo_param PRIVATE PARAM=1)
qt5_use_modules(Demo_param Widgets SerialPort Network)
target_link_libraries(Demo_param ${PROJECT_LIBS})

#add_executable(Demo_ICMOS WIN32 ${SOURCE_FILES})
#target_compile_definitions(Demo_ICMOS PRIVATE ICMOS=1)
#qt5_use_modules(Demo_ICMOS Widgets SerialPort Network)
#target_link_libraries(Demo_ICMOS ${PROJECT_LIBS})
