cmake_minimum_required(VERSION 3.21)

project(medAlert VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets DBus)

qt_add_executable(medAlert
    apptranslator.cpp
    apptranslator.h
    appsettings.cpp
    appsettings.h
    main.cpp
    mainwindow.cpp
    mainwindow.h
    medicinestore.cpp
    medicinestore.h
    notificationservice.cpp
    notificationservice.h
)

target_link_libraries(medAlert PRIVATE Qt6::Core Qt6::Widgets Qt6::DBus)

install(TARGETS medAlert RUNTIME DESTINATION bin)
install(FILES
    systemd/medalert.service
    systemd/medalert.timer
    DESTINATION share/medAlert/systemd)