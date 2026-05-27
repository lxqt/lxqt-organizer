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
find_package(Qt6 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS Widgets Concurrent DBus Svg LinguistTools)
find_package(KF6CalendarCore REQUIRED)
find_package(KF6Contacts REQUIRED)

include(GNUInstallDirs)
include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)
include(Qt6TranslationLoader)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

message(STATUS "Building ${PROJECT_NAME} ${LXQT_ORGANIZER_VERSION} with Qt ${Qt6_VERSION}")

include(src/domain/sources.cmake)
include(src/services/sources.cmake)
include(src/storage/api/sources.cmake)
include(src/storage/fs/sources.cmake)
include(src/storage/repo/sources.cmake)
include(src/ui/core/sources.cmake)
include(src/ui/messages/sources.cmake)
include(src/ui/models/sources.cmake)
include(src/ui/widgets/sources.cmake)
include(src/ui/dialogs/sources.cmake)
include(src/ui/panes/sources.cmake)
include(src/ui/shell/sources.cmake)
include(src/app/sources.cmake)

set(LXQT_ORGANIZER_TRANSLATABLE_SOURCES
    ${LXQT_ORGANIZER_APP_SOURCES}
    ${LXQT_ORGANIZER_APP_HEADERS}
    ${LXQT_ORGANIZER_CONTEXT_SOURCES}
    ${LXQT_ORGANIZER_CONTEXT_HEADERS}
    ${LXQT_ORGANIZER_UI_COLLECTIONS_SOURCES}
    ${LXQT_ORGANIZER_UI_COLLECTIONS_HEADERS}
    ${LXQT_ORGANIZER_UI_COLLECTION_UTILS_SOURCES}
    ${LXQT_ORGANIZER_UI_COLLECTION_UTILS_HEADERS}
    ${LXQT_ORGANIZER_UI_WIDGETS_SOURCES}
    ${LXQT_ORGANIZER_UI_WIDGETS_HEADERS}
    ${LXQT_ORGANIZER_UI_WIDGETS_FORMS}
    ${LXQT_ORGANIZER_UI_CORE_HEADERS}
    ${LXQT_ORGANIZER_UI_MESSAGES_SOURCES}
    ${LXQT_ORGANIZER_UI_MESSAGES_HEADERS}
    ${LXQT_ORGANIZER_UI_MODELS_SOURCES}
    ${LXQT_ORGANIZER_UI_MODELS_HEADERS}
    ${LXQT_ORGANIZER_UI_DIALOGS_SOURCES}
    ${LXQT_ORGANIZER_UI_DIALOGS_HEADERS}
    ${LXQT_ORGANIZER_UI_PANES_SOURCES}
    ${LXQT_ORGANIZER_UI_PANES_HEADERS}
    ${LXQT_ORGANIZER_UI_SHELL_SOURCES}
    ${LXQT_ORGANIZER_UI_SHELL_HEADERS}
    ${LXQT_ORGANIZER_DOMAIN_SOURCES}
    ${LXQT_ORGANIZER_DOMAIN_HEADERS}
    ${LXQT_ORGANIZER_STORAGE_API_SOURCES}
    ${LXQT_ORGANIZER_STORAGE_API_HEADERS}
    ${LXQT_ORGANIZER_STORAGE_FS_SOURCES}
    ${LXQT_ORGANIZER_STORAGE_FS_HEADERS}
    ${LXQT_ORGANIZER_STORAGE_REPO_SOURCES}
    ${LXQT_ORGANIZER_STORAGE_REPO_HEADERS}
    ${LXQT_ORGANIZER_SERVICES_SOURCES}
    ${LXQT_ORGANIZER_SERVICES_HEADERS}
)

set(LXQT_ORGANIZER_FORMS
    ${LXQT_ORGANIZER_UI_COLLECTIONS_FORMS}
    ${LXQT_ORGANIZER_UI_WIDGETS_FORMS}
    ${LXQT_ORGANIZER_UI_DIALOGS_FORMS}
    ${LXQT_ORGANIZER_UI_PANES_FORMS}
    ${LXQT_ORGANIZER_UI_SHELL_FORMS}
)

qt6_add_dbus_adaptor(LXQT_ORGANIZER_DBUS_SOURCES
    src/app/org.lxqt.LXQtOrganizer.Application.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/application.h
    LXQtOrganizerApplication
    lxqtorganizerapplicationadaptor
    LXQtOrganizerApplicationAdaptor
)
set_property(SOURCE ${LXQT_ORGANIZER_DBUS_SOURCES} PROPERTY SKIP_AUTOGEN ON)

# Translations **********************************
lxqt_translate_ts(LXQT_ORGANIZER_QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
        ${LXQT_ORGANIZER_TRANSLATABLE_SOURCES}
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

add_library(lxqt_organizer_domain STATIC
    ${LXQT_ORGANIZER_DOMAIN_SOURCES}
    ${LXQT_ORGANIZER_DOMAIN_HEADERS}
)

target_include_directories(lxqt_organizer_domain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/src/domain/codecs
)

target_link_libraries(lxqt_organizer_domain
    PUBLIC
        Qt6::Core
        KF6::CalendarCore
        KF6::Contacts
)

add_library(lxqt_organizer_storage_api INTERFACE)

target_sources(lxqt_organizer_storage_api
    INTERFACE
        ${LXQT_ORGANIZER_STORAGE_API_HEADERS}
)

target_include_directories(lxqt_organizer_storage_api
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/api
)

target_link_libraries(lxqt_organizer_storage_api
    INTERFACE
        lxqt_organizer_domain
)

add_library(lxqt_organizer_storage_fs STATIC
    ${LXQT_ORGANIZER_STORAGE_FS_SOURCES}
    ${LXQT_ORGANIZER_STORAGE_FS_HEADERS}
)

target_include_directories(lxqt_organizer_storage_fs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/fs
)

target_link_libraries(lxqt_organizer_storage_fs
    PUBLIC
        lxqt_organizer_domain
        lxqt_organizer_storage_api
    PRIVATE
        Qt6::Concurrent
)

add_library(lxqt_organizer_storage_repo STATIC
    ${LXQT_ORGANIZER_STORAGE_REPO_SOURCES}
    ${LXQT_ORGANIZER_STORAGE_REPO_HEADERS}
)

target_include_directories(lxqt_organizer_storage_repo
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/storage/repo
)

target_link_libraries(lxqt_organizer_storage_repo
    PUBLIC
        lxqt_organizer_domain
        lxqt_organizer_storage_api
    PRIVATE
        lxqt_organizer_storage_fs
)

add_library(lxqt_organizer_services STATIC
    ${LXQT_ORGANIZER_SERVICES_SOURCES}
    ${LXQT_ORGANIZER_SERVICES_HEADERS}
)

target_include_directories(lxqt_organizer_services
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services
)

target_link_libraries(lxqt_organizer_services
    PUBLIC
        lxqt_organizer_domain
        lxqt_organizer_storage_api
        lxqt_organizer_storage_repo
    PRIVATE
        lxqt_organizer_storage_fs
        Qt6::Concurrent
        lxqt
)

add_library(lxqt_organizer_ui_core INTERFACE)

target_sources(lxqt_organizer_ui_core
    INTERFACE
        ${LXQT_ORGANIZER_UI_CORE_HEADERS}
)

target_include_directories(lxqt_organizer_ui_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/core
)

target_link_libraries(lxqt_organizer_ui_core
    INTERFACE
        lxqt_organizer_domain
        lxqt_organizer_services
)

add_library(lxqt_organizer_ui_messages STATIC
    ${LXQT_ORGANIZER_UI_MESSAGES_SOURCES}
    ${LXQT_ORGANIZER_UI_MESSAGES_HEADERS}
)

target_include_directories(lxqt_organizer_ui_messages
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/messages
)

target_link_libraries(lxqt_organizer_ui_messages
    PUBLIC
        lxqt_organizer_services
        lxqt_organizer_storage_api
    PRIVATE
        Qt6::Widgets
)

add_library(lxqt_organizer_ui_widgets STATIC
    ${LXQT_ORGANIZER_UI_WIDGETS_SOURCES}
    ${LXQT_ORGANIZER_UI_WIDGETS_HEADERS}
    ${LXQT_ORGANIZER_UI_WIDGETS_FORMS}
)

target_include_directories(lxqt_organizer_ui_widgets
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets
)

target_link_libraries(lxqt_organizer_ui_widgets
    PUBLIC
        lxqt_organizer_domain
        Qt6::Widgets
)

add_library(lxqt_organizer_ui_models STATIC
    ${LXQT_ORGANIZER_UI_MODELS_SOURCES}
    ${LXQT_ORGANIZER_UI_MODELS_HEADERS}
)

target_include_directories(lxqt_organizer_ui_models
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/models
)

target_link_libraries(lxqt_organizer_ui_models
    PUBLIC
        lxqt_organizer_domain
        Qt6::Widgets
)

target_link_libraries(lxqt_organizer_ui_widgets
    PRIVATE
        lxqt_organizer_ui_models
)

add_library(lxqt_organizer_ui_dialogs STATIC
    ${LXQT_ORGANIZER_UI_DIALOGS_SOURCES}
    ${LXQT_ORGANIZER_UI_DIALOGS_HEADERS}
    ${LXQT_ORGANIZER_UI_DIALOGS_FORMS}
)

target_include_directories(lxqt_organizer_ui_dialogs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
)

target_link_libraries(lxqt_organizer_ui_dialogs
    PUBLIC
        lxqt_organizer_domain
        Qt6::Widgets
    PRIVATE
        lxqt_organizer_ui_widgets
)

add_library(lxqt_organizer_ui_panes STATIC
    ${LXQT_ORGANIZER_UI_PANES_SOURCES}
    ${LXQT_ORGANIZER_UI_PANES_HEADERS}
    ${LXQT_ORGANIZER_UI_PANES_FORMS}
)

target_include_directories(lxqt_organizer_ui_panes
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/panes
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/shell
)

target_link_libraries(lxqt_organizer_ui_panes
    PUBLIC
        lxqt_organizer_services
        lxqt_organizer_ui_core
        lxqt_organizer_ui_models
        Qt6::Widgets
    PRIVATE
        lxqt_organizer_ui_dialogs
        lxqt_organizer_ui_messages
        lxqt_organizer_ui_widgets
)

add_library(lxqt_organizer_context STATIC
    ${LXQT_ORGANIZER_CONTEXT_SOURCES}
    ${LXQT_ORGANIZER_CONTEXT_HEADERS}
)

target_include_directories(lxqt_organizer_context
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)

target_link_libraries(lxqt_organizer_context
    PUBLIC
        lxqt_organizer_ui_core
        lxqt_organizer_services
    PRIVATE
        lxqt_organizer_storage_fs
)

add_library(lxqt_organizer_ui_shell STATIC
    ${LXQT_ORGANIZER_UI_SHELL_SOURCES}
    ${LXQT_ORGANIZER_UI_SHELL_HEADERS}
    ${LXQT_ORGANIZER_UI_SHELL_FORMS}
)

target_include_directories(lxqt_organizer_ui_shell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/shell
)

target_compile_definitions(lxqt_organizer_ui_shell
    PRIVATE
        LXQT_ORGANIZER_VERSION="${LXQT_ORGANIZER_VERSION}"
)

target_link_libraries(lxqt_organizer_ui_shell
    PUBLIC
        lxqt_organizer_ui_panes
        lxqt_organizer_ui_core
        lxqt_organizer_ui_models
        Qt6::Widgets
    PRIVATE
        lxqt_organizer_ui_dialogs
        lxqt_organizer_ui_messages
        lxqt_organizer_ui_widgets
        lxqt
)

foreach(layer_target
        lxqt_organizer_domain
        lxqt_organizer_storage_fs
        lxqt_organizer_storage_repo
        lxqt_organizer_services
        lxqt_organizer_ui_messages
        lxqt_organizer_ui_widgets
        lxqt_organizer_ui_models
        lxqt_organizer_ui_dialogs
        lxqt_organizer_ui_panes
        lxqt_organizer_context
        lxqt_organizer_ui_shell)
    set_property(TARGET ${layer_target} PROPERTY CXX_STANDARD 17)
    set_property(TARGET ${layer_target} PROPERTY CXX_STANDARD_REQUIRED ON)
endforeach()

add_executable(${PROJECT_NAME}
    ${LXQT_ORGANIZER_APP_SOURCES}
    ${LXQT_ORGANIZER_APP_HEADERS}
    ${LXQT_ORGANIZER_DBUS_SOURCES}
    ${LXQT_ORGANIZER_QM_FILES}
    ${LXQT_ORGANIZER_QM_LOADER}
    ${DESKTOP_FILES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app
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
        lxqt_organizer_context
        lxqt_organizer_ui_shell
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::DBus
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
