cmake_minimum_required(VERSION 3.16)

project(PoeLifer VERSION 0.1 LANGUAGES CXX)

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
    planwidget.cpp
    planwidget.h
    screenpicker.cpp
    screenpicker.h
    networkutil.cpp
    networkutil.h
    keycapturedialog.cpp
    keycapturedialog.h
    globalhook.cpp
    globalhook.h
)

if(WIN32)
    list(APPEND PROJECT_SOURCES app.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PoeLifer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        .gitignore
    )
else()
    if(ANDROID)
        add_library(PoeLifer SHARED ${PROJECT_SOURCES})
    else()
        add_executable(PoeLifer ${PROJECT_SOURCES})
    endif()
endif()

target_link_libraries(PoeLifer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(WIN32)
    target_link_libraries(PoeLifer PRIVATE iphlpapi psapi)
endif()

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.PoeLifer)
endif()

set_target_properties(PoeLifer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS PoeLifer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(PoeLifer)
endif()