cmake_minimum_required(VERSION 3.5)

project(GLI_user_panel LANGUAGES CXX VERSION 0.6.1.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_STATIC 0)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SDK_DIR "${PROJECT_SOURCE_DIR}/../SDK/sdk64")
else()
    set(SDK_DIR "${PROJECT_SOURCE_DIR}/../SDK/sdk32")
endif()

set(Hik_SDK_DIR "${SDK_DIR}/mvs")
set(HQV_DIR "${SDK_DIR}/HQV")
set(EURESYS_DIR "${SDK_DIR}/euresys")
set(OpenCV_DIR "${SDK_DIR}/opencv")
set(FFMPEG_DIR "${SDK_DIR}/ffmpeg")

include_directories(${SDK_DIR}/VLD/include)
link_directories(${SDK_DIR}/VLD/lib)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

find_package(QT NAMES Qt5 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets SerialPort Network DataVisualization Concurrent REQUIRED)

if (WIN32)
    include_directories(${Hik_SDK_DIR}/include/win)
    include_directories(${HQV_DIR}/include)
    include_directories(${EURESYS_DIR}/include)
    include_directories(${OpenCV_DIR}/include)
    include_directories(${FFMPEG_DIR}/include)

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        link_directories(${Hik_SDK_DIR}/lib/x64)
        link_directories(${HQV_DIR}/lib)
        link_directories(${EURESYS_DIR}/lib)
        link_directories(${FFMPEG_DIR}/lib/x64)
        if (BUILD_STATIC)
            link_directories(${OpenCV_DIR}/lib/x64_static)
        else()
            link_directories(${OpenCV_DIR}/lib/x64)
        endif()
    else()
        link_directories(${Hik_SDK_DIR}/lib/x86)
        link_directories(${HQV_DIR}/lib)
        link_directories(${EURESYS_DIR}/lib)
        link_directories(${OpenCV_DIR}/lib/x86)
    endif()
elseif (APPLE)

# for aarch64 only
elseif (UNIX)
    include_directories(${PROJECT_SOURCE_DIR}/src)

    include_directories(${Hik_SDK_DIR}/include/linux)
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
    configure_file("${CMAKE_SOURCE_DIR}/src/version.h.in" "${CMAKE_SOURCE_DIR}/src/version.h" NEWLINE_STYLE WIN32)
endif()

# for now
set(SOURCE_FILES
    src/main.cpp
    src/utils.h
    src/utils.cpp
    src/userpanel.h
    src/userpanel.cpp
    src/user_panel.ui
    src/threadpool.h
    src/threadpool.cpp
    src/mywidget.h
    src/mywidget.cpp
#    src/progsettings.h
#    src/progsettings.cpp
#    src/settings.ui
    src/lasersettings.cpp
    src/lasersettings.h
    src/laser_settings.ui
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
    src/preferences.h
    src/preferences.cpp
    src/preferences.ui
    src/scanconfig.h
    src/scanconfig.cpp
    src/scanconfig.ui
    src/controlport.h
    src/controlport.cpp
#    src/videosurface.h
#    src/videosurface.cpp
    src/distance3dview.h
    src/distance3dview.cpp
    src/distance3dview.ui
    src/aliasing.h
    src/aliasing.cpp
    resources/resources.qrc
    resources/style_dark.qss
    resources/style_light.qss
)

if (WIN32)
    set(SOURCE_FILES
        ${SOURCE_FILES}
        resources/gist.rc
    )
endif()

set(PROJECT_LIBS
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::DataVisualization
    Qt${QT_VERSION_MAJOR}::Concurrent
    # mvs
    MvCameraControl
    # Euresys
#    MultiCam
#    clallserial
)

#if (CMAKE_SIZEOF_VOID_P EQUAL 8)
#    set(PROJECT_LIBS
#        ${PROJECT_LIBS}
#        # HQV
#        HQV_AVIProcess64
#        HQV_DataConvert64
#        HQV_SDK64
#    )
#else()
#    set(PROJECT_LIBS
#        ${PROJECT_LIBS}
#        # HQV
#        HQV_AVIProcess
#        HQV_DataConvert
#        HQV_SDK
#    )
#endif()

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
        # ffmpeg
        avcodec
        avformat
        avutil
        avfilter
        swscale
        swresample
        postproc
        libx264
        # win lib for ffmpeg
        Strmiids
        Mfuuid
        ws2_32
        Secur32
        Bcrypt
    )
    if (BUILD_STATIC)
        string(LENGTH ${QT_DIR} LEN_QT_DIR)
        math(EXPR LEN_QT_DIR ${LEN_QT_DIR}-14)
        string(SUBSTRING ${QT_DIR} 0 ${LEN_QT_DIR} QT_ROOT_DIR)
        link_directories(${QT_ROOT_DIR}/lib)
        link_directories(${QT_ROOT_DIR}/plugins/platforms)
        set(PROJECT_LIBS
            ${PROJECT_LIBS}
            # windows static runtime lib
            setupapi      #QtSerialPort qserialportinfo_win.obj
            ws2_32        #Qt5Network qhostaddress.obj qhostinfo_win.obj qnativesocketengine.obj qnativesocketengine_win.obj
            iphlpapi      #Qt5Network qnetworkinterface_win.obj
            qtpcre2       #Qt5Core qregularexpression.obj
            netapi32      #Qt5Core qfilesystemengine_win.obj
            userenv       #Qt5Core qfilesystemengine_win.obj
            mincore       #Qt5Core qcoreapplication.obj
            qtharfbuzz    #Qt5Gui qtextengine.obj
            qtlibpng      #QtGui qpnghandler.obj
            ippicvmt      #opencv_core454
            ippiw         #opencv_core454
            ittnotify     #opencv_core454
            zlib          #opencv_core454
            libopenjp2    #opencv_imgcodecs454
            libjpeg-turbo #opencv_imgcodecs454
            libtiff       #opencv_imgcodecs454
            libwebp       #opencv_imgcodecs454
            IlmImf        #opencv_imgcodecs454
            qwindows      #platform plugin
        )
    endif()
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

if (MSVC AND BUILD_STATIC)
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
endif()

add_executable(GLI_params WIN32 ${TRANSLATE} ${SOURCE_FILES})
target_compile_definitions(GLI_params PRIVATE PARAM=1)
qt5_use_modules(GLI_params Widgets SerialPort Network DataVisualization Concurrent)
target_link_libraries(GLI_params ${PROJECT_LIBS})

add_executable(ICMOS WIN32 ${TRANSLATE} ${SOURCE_FILES})
target_compile_definitions(ICMOS PRIVATE ICMOS=1)
qt5_use_modules(ICMOS Widgets SerialPort Network DataVisualization Concurrent)
target_link_libraries(ICMOS ${PROJECT_LIBS})

add_executable(LvTong WIN32 ${TRANSLATE} ${SOURCE_FILES})
target_compile_definitions(LvTong PRIVATE LVTONG=1)
qt5_use_modules(LvTong Widgets SerialPort Network DataVisualization Concurrent)
target_link_libraries(LvTong ${PROJECT_LIBS} opencv_dnn454)
