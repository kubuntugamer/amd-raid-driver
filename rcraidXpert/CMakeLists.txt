cmake_minimum_required(VERSION 3.16)
project(rcraidXpert VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(rcraidXpert
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow_layout.cpp
    src/mainwindow_actions.cpp
    src/mainwindow_telemetry.cpp
    src/modules/disktablewidget.cpp
    src/modules/disktablewidget.h
    src/modules/arrayoptionswidget.cpp
    src/modules/arrayoptionswidget.h
    src/modules/statuspanelwidget.cpp
    src/modules/statuspanelwidget.h
    src/modules/dashboardwidget.cpp
    src/modules/dashboardwidget.h
    src/modules/taskmanagerwidget.cpp
    src/modules/taskmanagerwidget.h
    src/modules/createarraywizard.cpp
    src/modules/createarraywizard.h
    src/modules/smartmonitorwidget.cpp
    src/modules/smartmonitorwidget.h
    src/modules/backgroundrateswidget.cpp
    src/modules/backgroundrateswidget.h
    src/modules/spareassignmentdialog.cpp
    src/modules/spareassignmentdialog.h
    src/modules/arraydatacontroller.cpp
    src/modules/arraydatacontroller.h
)

target_link_libraries(rcraidXpert PRIVATE Qt6::Widgets)
