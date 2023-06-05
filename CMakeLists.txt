cmake_minimum_required(VERSION 3.5)

project(wsted VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Core5Compat)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Core5Compat)

set(CLIENT_PROJECT_SOURCES
    src/client/main.cpp
    src/client/loginwindow.hpp src/client/loginwindow.cpp
    src/client/roomwindow.hpp src/client/roomwindow.cpp
    src/logger.hpp src/logger.cpp
    resources/ui.qrc
)

set(SERVER_PROJECT_SOURCES
    src/server/main.cpp
    src/server/server.hpp src/server/server.cpp
    src/logger.hpp src/logger.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(wsted-client
        MANUAL_FINALIZATION
        ${CLIENT_PROJECT_SOURCES}
    )

    qt_add_executable(wsted-server
        MANUAL_FINALIZATION
        ${SERVER_PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET wsted APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(wsted-client SHARED
            ${CLIENT_PROJECT_SOURCES}
        )

        add_library(wsted-server SHARED
            ${SERVER_PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(wsted-client
            ${CLIENT_PROJECT_SOURCES}
        )

        add_executable(wsted-server
            ${SERVER_PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(wsted-client PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Core5Compat)
target_link_libraries(wsted-server PRIVATE Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Core5Compat)

set_target_properties(wsted-client PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER wsted.client.id
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

set_target_properties(wsted-server PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER wsted.server.id
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_options(wsted-client PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(wsted-server PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS wsted-client wsted-server
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(wsted-client)
    qt_finalize_executable(wsted-server)
endif()
