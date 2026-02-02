cmake_minimum_required(VERSION 3.5)

project(GLI_user_panel LANGUAGES CXX VERSION 0.10.1.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_STATIC 0)
set(USING_CAMERALINK 0)
set(DISTANCE_3D_VIEW 0)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SDK_DIR "${PROJECT_SOURCE_DIR}/../SDK/sdk64")
else()
    set(SDK_DIR "${PROJECT_SOURCE_DIR}/../SDK/sdk32")
endif()

set(Hik_SDK_DIR "${SDK_DIR}/mvs")
set(HQV_DIR "${SDK_DIR}/HQV")
set(EURESYS_DIR "${SDK_DIR}/euresys")
set(EIGEN_DIR "${SDK_DIR}/eigen")
set(JSON_DIR "${SDK_DIR}/nlohmann-json")
if(WIN32)
    set(eBUS_DIR "${SDK_DIR}/eBUS")
    set(OpenCV_DIR "${SDK_DIR}/opencv")
    set(FFmpeg_DIR "${SDK_DIR}/ffmpeg")
    set(GCAN_DIR "${SDK_DIR}/GCAN")
    # YOLO/ONNX Runtime configuration (using SDK)
    set(ONNXRUNTIME_DIR "${SDK_DIR}/onnxruntime-gpu")
else()
    set(OpenCV_DIR "/opt/OpenCV4.8.0")
    set(FFmpeg_DIR "/opt/FFmpeg5.1.3")

    find_package(Eigen3)
    find_package(OpenCV REQUIRED)

    find_package(PkgConfig REQUIRED)
    set(ENV{PKG_CONFIG_PATH} "${FFmpeg_DIR}/lib/pkgconfig")
    pkg_check_modules(LIBFFMPEG REQUIRED IMPORTED_TARGET
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
    )
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

if(WIN32)
    include_directories(${SDK_DIR}/VLD/include)
    link_directories(${SDK_DIR}/VLD/lib)
endif()

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

find_package(QT NAMES Qt5 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets SerialPort Network REQUIRED)
if (DISTANCE_3D_VIEW)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DataVisualization REQUIRED)
endif(DISTANCE_3D_VIEW)

#message(${CMAKE_HOST_SYSTEM_PROCESSOR})

if(WIN32)
    include_directories(${PROJECT_SOURCE_DIR}/src)

    include_directories(${Hik_SDK_DIR}/include/win)
    include_directories(${HQV_DIR}/include)
    include_directories(${EURESYS_DIR}/include)
    include_directories(${OpenCV_DIR}/include)
    include_directories(${FFmpeg_DIR}/include)
    include_directories(${eBUS_DIR}/include)
    include_directories(${EIGEN_DIR}/include)
    include_directories(${JSON_DIR}/include)
    include_directories(${GCAN_DIR}/include)
    include_directories(${ONNXRUNTIME_DIR}/include/win)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        link_directories(${Hik_SDK_DIR}/lib/x64)
        link_directories(${HQV_DIR}/lib)
        link_directories(${EURESYS_DIR}/lib)
        link_directories(${FFmpeg_DIR}/lib/x64)
        if(BUILD_STATIC)
            link_directories(${OpenCV_DIR}/lib/x64_static)
        else()
            link_directories(${OpenCV_DIR}/lib/x64)
        endif()
        link_directories(${eBUS_DIR}/lib)
        link_directories(${GCAN_DIR}/lib)
        link_directories(${ONNXRUNTIME_DIR}/lib/x64)
    else()
        link_directories(${Hik_SDK_DIR}/lib/x86)
        link_directories(${HQV_DIR}/lib)
        link_directories(${EURESYS_DIR}/lib)
        link_directories(${OpenCV_DIR}/lib/x86)
        link_directories(${eBUS_DIR}/lib)
    endif()
elseif(APPLE)

elseif(UNIX)
    include_directories(${PROJECT_SOURCE_DIR}/src)

    include_directories(${Hik_SDK_DIR}/include/linux)
    include_directories(${HQV_DIR}/include)
    include_directories(${EURESYS_DIR}/include)
    include_directories(${OpenCV_DIR}/include)
    include_directories(${FFmpeg_DIR}/include/linux)

    link_directories(${HQV_DIR}/lib)
    link_directories(${EURESYS_DIR}/lib)
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
        link_directories(${Hik_SDK_DIR}/lib/aarch64)
        link_directories(${OpenCV_DIR}/lib/aarch64)
        link_directories(${FFmpeg_DIR}/lib/aarch64)
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
        link_directories(${Hik_SDK_DIR}/lib/x64)
        link_directories(${OpenCV_DIR}/lib/x64)
        link_directories(${FFmpeg_DIR}/lib/x64)
    endif()
endif()

if(MSVC)
    set(VER_PRELEASE 1)
    configure_file("${CMAKE_SOURCE_DIR}/src/cmake/gist.rc.in" "${CMAKE_SOURCE_DIR}/resources/gist.rc" NEWLINE_STYLE WIN32)
    configure_file("${CMAKE_SOURCE_DIR}/src/cmake/version.h.in" "${CMAKE_SOURCE_DIR}/src/util/version.h" NEWLINE_STYLE WIN32)
endif()

# for now
set(SOURCE_FILES
    src/main.cpp
    src/util/util.h
    src/util/util.cpp
    src/util/threadpool.h
    src/util/threadpool.cpp
    src/util/config.h
    src/util/config.cpp
    src/widgets/mywidget.h
    src/widgets/mywidget.cpp
    src/visual/userpanel.h
    src/visual/userpanel.cpp
    src/visual/user_panel.ui
    src/visual/preferences.h
    src/visual/preferences.cpp
    src/visual/preferences.ui
    src/visual/scanconfig.h
    src/visual/scanconfig.cpp
    src/visual/scanconfig.ui
    src/visual/lasercontrol.cpp
    src/visual/lasercontrol.h
    src/visual/laser_control.ui
    src/visual/developeroptions.h
    src/visual/developeroptions.cpp
    src/visual/developeroptions.ui
    src/visual/serialserver.h
    src/visual/serialserver.cpp
    src/visual/serialserver.ui
    src/visual/presetpanel.h
    src/visual/presetpanel.cpp
    src/visual/aliasing.h
    src/visual/aliasing.cpp
    src/image/imageproc.h
    src/image/imageproc.cpp
    src/image/imageio.h
    src/image/imageio.cpp
    src/cam/cam.h
    src/cam/mvcam.h
    src/cam/mvcam.cpp
    src/thread/joystick.h
    src/thread/joystick.cpp
#    src/thread/controlportthread.h
#    src/thread/controlportthread.cpp
    src/port/controlport.h
    src/port/controlport.cpp
    src/port/tcu.h
    src/port/tcu.cpp
    src/port/lens.h
    src/port/lens.cpp
    src/port/laser.h
    src/port/laser.cpp
    src/port/ptz.h
    src/port/ptz.cpp
    src/port/rangefinder.h
    src/port/rangefinder.cpp
    src/port/huanyu.h
    src/port/huanyu.cpp
    src/port/usbcan.h
    src/port/usbcan.cpp
    src/port/udpptz.h
    src/port/udpptz.cpp
    src/automation/scanpreset.h
    src/automation/scanpreset.cpp
    src/automation/autoscan.h
    src/automation/autoscan.cpp
    src/yolo/yolo_detector.h
    src/yolo/yolo_detector.cpp
    src/yolo/inference.h
    src/yolo/inference.cpp
    src/yolo/yolo_app_config.h
    resources/resources.qrc
    resources/style_dark.qss
    resources/style_light.qss
#    src/progsettings.h
#    src/progsettings.cpp
#    src/settings.ui
#    src/cam.cpp
#    src/hqvcam.h
#    src/hqvcam.cpp
#    src/euresyscam.h
#    src/euresyscam.cpp
#    src/videosurface.h
#    src/videosurface.cpp
)

if (DISTANCE_3D_VIEW)
    set(SOURCE_FILES
        ${SOURCE_FILES}
        src/visual/distance3dview.h
        src/visual/distance3dview.cpp
        src/visual/distance3dview.ui
    )
endif(DISTANCE_3D_VIEW)

if(WIN32)
    set(SOURCE_FILES
        ${SOURCE_FILES}
        src/cam/ebuscam.h
        src/cam/ebuscam.cpp
        resources/gist.rc
    )
endif(WIN32)

if(USING_CAMERALINK)
    set(SOURCE_FILES
        ${SOURCE_FILES}
        src/cam/euresyscam.h
        src/cam/euresyscam.cpp
    )
endif(USING_CAMERALINK)

set(PROJECT_LIBS
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Network
#    Qt${QT_VERSION_MAJOR}::Concurrent
    # mvs
    MvCameraControl
    # Euresys
    MultiCam
#   clallserial
)
if (DISTANCE_3D_VIEW)
    set(PROJECT_LIBS
        ${PROJECT_LIBS}
        Qt${QT_VERSION_MAJOR}::DataVisualization
    )
endif(DISTANCE_3D_VIEW)

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

if(WIN32)
    set(PROJECT_LIBS
        ${PROJECT_LIBS}
        # joystick
        winmm
        # opencv
        opencv_core454
        opencv_imgcodecs454
        opencv_imgproc454
        opencv_videoio454
        opencv_dnn454
#        opencv_dnn_superres454
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
        # gcan
        ECanVci64
        # onnx runtime for yolo
        onnxruntime
    )
    if(BUILD_STATIC)
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
elseif(APPLE)

# for aarch64 only
elseif(UNIX)
    set(PROJECT_LIBS
        ${PROJECT_LIBS}
        # ffmpeg
        PkgConfig::LIBFFMPEG
#        avcodec
#        avformat
#        avutil
#        avfilter
#        swscale
#        swresample
#        postproc
        # opencv
        opencv_core
        opencv_imgcodecs
        opencv_imgproc
        opencv_videoio
        # pthread
        pthread
    )
endif()

if(MSVC AND BUILD_STATIC)
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

add_library(ImgProc STATIC src/image/imageproc.h src/image/imageproc.cpp)
if(WIN32)
    target_link_libraries(ImgProc opencv_core454 opencv_imgcodecs454 opencv_imgproc454)
else()
    target_link_libraries(ImgProc opencv_core opencv_imgcodecs opencv_imgproc)
endif()

add_executable(GLI_params WIN32 ${TRANSLATE} ${SOURCE_FILES})
target_compile_definitions(GLI_params PRIVATE PARAM=1)
if(USING_CAMERALINK)
    target_compile_definitions(GLI_params PRIVATE USING_CAMERALINK=1)
endif()
qt5_use_modules(GLI_params Widgets SerialPort Network)
if (DISTANCE_3D_VIEW)
    qt5_use_modules(GLI_params DataVisualization)
    target_compile_definitions(GLI_params PRIVATE DISTANCE_3D_VIEW=1)
endif(DISTANCE_3D_VIEW)
target_link_libraries(GLI_params ${PROJECT_LIBS})

#add_executable(LvTong WIN32 ${TRANSLATE} ${SOURCE_FILES})
#target_compile_definitions(LvTong PRIVATE LVTONG=1)
#if(USING_CAMERALINK)
#    target_compile_definitions(LvTong PRIVATE USING_CAMERALINK=1)
#endif()
#qt5_use_modules(LvTong Widgets SerialPort Network)
#if (DISTANCE_3D_VIEW)
#    qt5_use_modules(LvTong DataVisualization)
#    target_compile_definitions(LvTong PRIVATE DISTANCE_3D_VIEW=1)
#endif(DISTANCE_3D_VIEW)
#if(WIN32)
#    target_link_libraries(LvTong ${PROJECT_LIBS} opencv_dnn454)
#else()
#    target_link_libraries(LvTong ${PROJECT_LIBS} opencv_dnn)
#endif()
#target_link_libraries(LvTong ${PROJECT_LIBS})
