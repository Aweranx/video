cmake_minimum_required(VERSION 3.16)

project(videoplay_vlc VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(videoplay_vlc
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        vlcplayer.h vlcplayer.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET videoplay_vlc APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(videoplay_vlc SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(videoplay_vlc
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# 设置 VLC SDK 的路径
set(VLC_SDK_PATH "D:/IDE/vlc-3.0.22/sdk")

# 添加头文件包含路径
target_include_directories(videoplay_vlc PRIVATE
    ${VLC_SDK_PATH}/include
)
# 链接 VLC 库文件 (以及 Qt 的库)
target_link_libraries(videoplay_vlc PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${VLC_SDK_PATH}/lib/libvlc.lib
    ${VLC_SDK_PATH}/lib/libvlccore.lib
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.videoplay_vlc)
endif()
set_target_properties(videoplay_vlc PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS videoplay_vlc
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(videoplay_vlc)
endif()
