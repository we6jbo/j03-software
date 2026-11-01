cmake_minimum_required(VERSION 3.21)
project(tenvocalix VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
qt_add_executable(tenvocalix
    main.cpp mainwindow.cpp mainwindow.h mainwindow.ui resources.qrc
)
target_link_libraries(tenvocalix PRIVATE Qt6::Widgets Qt6::Multimedia)
install(TARGETS tenvocalix RUNTIME DESTINATION bin)
