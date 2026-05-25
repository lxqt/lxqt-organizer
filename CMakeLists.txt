cmake_minimum_required(VERSION 3.18.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-organizer)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)

# Version
set(LXQT_ORGANIZER_VERSION 0.7.4)

# Minimum Versions
set(LXQTBT_MINIMUM_VERSION "2.4.0")
set(LXQT_MINIMUM_VERSION "2.4.0")
set(QT_MINIMUM_VERSION "6.6.0")

find_package(lxqt2-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(Qt6 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS Widgets DBus Sql Xml Multimedia Svg LinguistTools)

include(GNUInstallDirs)
include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)
include(Qt6TranslationLoader)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

message(STATUS "Building ${PROJECT_NAME} ${LXQT_ORGANIZER_VERSION} with Qt ${Qt6_VERSION}")

set(LXQT_ORGANIZER_HEADERS
    src/dialogappointment.h
    src/dialogappointmentupdate.h
    src/dialogcontact.h
    src/dialogpreferences.h
    src/dialogrepeatappointment.h
    src/dialogupcomingbirthdays.h
    src/dialogupcomingschedule.h
    src/mainwindow.h
    src/lxqtorganizerapplication.h
    src/proxymodelcontacts.h
)

set(LXQT_ORGANIZER_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/birthdaylistmodel.cpp
    src/contactmodel.cpp
    src/daylistmodel.cpp
    src/dbmanager.cpp
    src/dialogappointment.cpp
    src/dialogappointmentupdate.cpp
    src/dialogcontact.cpp
    src/dialogpreferences.cpp
    src/dialogrepeatappointment.cpp
    src/dialogupcomingbirthdays.cpp
    src/dialogupcomingschedule.cpp
    src/lxqtorganizerapplication.cpp
    src/proxymodelcontacts.cpp
    src/schedulelistmodel.cpp
)

set(LXQT_ORGANIZER_FORMS
    src/dialogappointment.ui
    src/dialogappointmentupdate.ui
    src/dialogcontact.ui
    src/dialogpreferences.ui
    src/dialogrepeatappointment.ui
    src/dialogupcomingbirthdays.ui
    src/dialogupcomingschedule.ui
    src/mainwindow.ui
)

set(LXQT_ORGANIZER_RESOURCES
    src/sounds.qrc
)

qt6_add_dbus_adaptor(LXQT_ORGANIZER_DBUS_SOURCES
    src/org.lxqt.LXQtOrganizer.Application.xml
    src/lxqtorganizerapplication.h
    LXQtOrganizerApplication
    lxqtorganizerapplicationadaptor
    LXQtOrganizerApplicationAdaptor
)
set_property(SOURCE ${LXQT_ORGANIZER_DBUS_SOURCES} PROPERTY SKIP_AUTOGEN ON)

# Translations **********************************
lxqt_translate_ts(LXQT_ORGANIZER_QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
        ${LXQT_ORGANIZER_SOURCES}
        ${LXQT_ORGANIZER_HEADERS}
        ${LXQT_ORGANIZER_FORMS}
    TRANSLATION_DIR
        src/translations
    INSTALL_DIR
        "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations"
)

qt6_translation_loader(LXQT_ORGANIZER_QM_LOADER
    "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}/translations"
    ${PROJECT_NAME}
)

lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/lxqt-organizer.desktop.in"
    TRANSLATION_DIR
        src/translations
    USE_YAML
)
#************************************************

add_executable(${PROJECT_NAME}
    ${LXQT_ORGANIZER_SOURCES}
    ${LXQT_ORGANIZER_HEADERS}
    ${LXQT_ORGANIZER_FORMS}
    ${LXQT_ORGANIZER_RESOURCES}
    ${LXQT_ORGANIZER_DBUS_SOURCES}
    ${LXQT_ORGANIZER_QM_FILES}
    ${LXQT_ORGANIZER_QM_LOADER}
    ${DESKTOP_FILES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        LXQT_ORGANIZER_VERSION="${LXQT_ORGANIZER_VERSION}"
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Widgets
        Qt6::DBus
        Qt6::Sql
        Qt6::Xml
        Qt6::Multimedia
        Qt6::Svg
        lxqt
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Runtime
)
install(FILES src/icons/lxqt-organizer.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    COMPONENT Runtime
)
install(FILES ${DESKTOP_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    COMPONENT Runtime
)
