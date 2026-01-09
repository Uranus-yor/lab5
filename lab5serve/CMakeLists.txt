cmake_minimum_required(VERSION 3.19)
project(lab5serve LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Sql)

qt_standard_project_setup()

qt_add_executable(lab5serve
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    TcpServer.cpp
    TcpServer.h
    ServerWindow.h
    ServerDbManager.h
)

target_link_libraries(lab5serve PRIVATE Qt::Core Qt::Gui Qt::Widgets Qt::Network Qt::Sql)

include(GNUInstallDirs)

install(TARGETS lab5serve
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET lab5serve
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
