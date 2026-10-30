cmake_minimum_required(VERSION 3.20)
project(VistaraViewer VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(Qt6 QUIET COMPONENTS OpenGLWidgets)
qt_standard_project_setup()

set(VISTARA_SOURCES
    main.cpp
    mainwindow.h mainwindow.cpp
    imageview.h imageview.cpp
    vistara.qrc
)
if(WIN32)
    list(APPEND VISTARA_SOURCES vistara.rc)
endif()

qt_add_executable(Vistara ${VISTARA_SOURCES})
if(WIN32)
    set_target_properties(Vistara PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
target_link_libraries(Vistara PRIVATE Qt6::Widgets Qt6::Network)
if(TARGET Qt6::OpenGLWidgets)
    target_link_libraries(Vistara PRIVATE Qt6::OpenGLWidgets)
    target_compile_definitions(Vistara PRIVATE VISTARA_HAS_OPENGL=1)
endif()
