cmake_minimum_required(VERSION 3.16)

project(ui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(ZLIB QUIET)

set(PROJECT_SOURCES
        src/app/main.cpp
        src/app/mainwindow.cpp
        src/app/mainwindow.h
        src/app/mainwindow.ui
        src/core/manage.h
        src/core/manage.cpp
        src/core/CoreTypes.h
        src/core/Serializable.h
        src/core/SerializationUtils.h
        src/core/SerializationUtils.cpp
        src/core/Author.h
        src/core/Author.cpp
        src/core/Source.h
        src/core/Journal.h
        src/core/Journal.cpp
        src/core/Conference.h
        src/core/Conference.cpp
        src/core/Attachment.h
        src/core/Attachment.cpp
        src/core/Paper.h
        src/core/Paper.cpp
        src/core/Catalog.h
        src/core/Catalog.cpp
        src/core/LibraryException.h
        src/core/LibraryManager.h
        src/core/LibraryManager.cpp
        src/pages/authorpage.h
        src/pages/authorpage.cpp
        src/pages/sourcepage.h
        src/pages/sourcepage.cpp
        src/pages/paperpage.h
        src/pages/paperpage.cpp
        src/pages/attachmentpage.h
        src/pages/attachmentpage.cpp
        src/pages/catalogpage.h
        src/pages/catalogpage.cpp
        src/dialogs/authordialog.h
        src/dialogs/authordialog.cpp
        src/dialogs/sourcedialog.h
        src/dialogs/sourcedialog.cpp
        src/dialogs/paperdialog.h
        src/dialogs/paperdialog.cpp
        src/dialogs/attachmentdialog.h
        src/dialogs/attachmentdialog.cpp
        src/dialogs/catalogdialog.h
        src/dialogs/catalogdialog.cpp
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pages
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ui APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ui SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ui
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(ui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
if(ZLIB_FOUND)
    target_link_libraries(ui PRIVATE ZLIB::ZLIB)
elseif(MINGW)
    target_link_libraries(ui PRIVATE z)
endif()

if(WIN32 AND QT_VERSION_MAJOR EQUAL 6)
    get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ui POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --release
                    --compiler-runtime
                    --no-translations
                    --no-system-d3d-compiler
                    --no-opengl-sw
                    "$<TARGET_FILE:ui>"
            COMMENT "Deploying Qt runtime dependencies"
        )
    endif()
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ui)
endif()
set_target_properties(ui PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ui)
endif()
