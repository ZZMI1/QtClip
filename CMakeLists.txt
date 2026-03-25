cmake_minimum_required(VERSION 3.16)

project(QtClip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(QTCLIP_QMAKE_EXECUTABLE "D:/Qtproject/thirdparty_install/vcpkg/installed/x64-windows/tools/qt5/bin/qmake.exe")
get_filename_component(QTCLIP_QT_BIN_DIR "${QTCLIP_QMAKE_EXECUTABLE}" DIRECTORY)
set(QTCLIP_WINDEPLOYQT_EXECUTABLE "${QTCLIP_QT_BIN_DIR}/windeployqt.exe")
set(QTCLIP_VCPKG_BIN_DIR "D:/Qtproject/thirdparty_install/vcpkg/installed/x64-windows/bin")

set(QTCLIP_VCPKG_QT_PREFIX "D:/Qtproject/thirdparty_install/vcpkg/installed/x64-windows/share/cmake")
if (EXISTS "${QTCLIP_VCPKG_QT_PREFIX}/Qt5/Qt5Config.cmake")
    list(PREPEND CMAKE_PREFIX_PATH "${QTCLIP_VCPKG_QT_PREFIX}")
endif()

find_package(Qt5 REQUIRED COMPONENTS Concurrent Core Network Sql Widgets)

set(QTCLIP_CORE_DATABASE_SOURCES
    src/core/database/qcdatabasemanager.cpp
    src/core/database/qcdatabaseschema.cpp
    src/core/database/qcdbenumcodec.cpp
)

set(QTCLIP_DOMAIN_ENTITY_SOURCES
    src/domain/entities/qcstudysession.cpp
    src/domain/entities/qcsnippet.cpp
    src/domain/entities/qcattachment.cpp
    src/domain/entities/qctag.cpp
    src/domain/entities/qcairecord.cpp
    src/domain/entities/qcappsetting.cpp
    src/domain/entities/qcexportrecord.cpp
)

set(QTCLIP_REPOSITORY_SQLITE_SOURCES
    src/repository/sqlite/qcsessionrepositorysqlite.cpp
    src/repository/sqlite/qcsnippetrepositorysqlite.cpp
    src/repository/sqlite/qcairecordrepositorysqlite.cpp
    src/repository/sqlite/qcsettingsrepositorysqlite.cpp
    src/repository/sqlite/qctagrepositorysqlite.cpp
)

set(QTCLIP_SERVICE_SOURCES
    src/services/qcaiprovider.cpp
    src/services/qcopenaicompatibleprovider.cpp
    src/services/qcmockaiprovider.cpp
    src/services/qcaiprocessservice.cpp
    src/services/qcaiservice.cpp
    src/services/qcexportdataservice.cpp
    src/services/qcmdexportrenderer.cpp
    src/services/qcmdexportservice.cpp
    src/services/qcscreencaptureservice.cpp
    src/services/qcsessionservice.cpp
    src/services/qcsettingsservice.cpp
    src/services/qcsnippetservice.cpp
    src/services/qctagservice.cpp
)

set(QTCLIP_UI_SOURCES
    src/ui/common/qcuilocalization.cpp
    src/ui/common/qcuitheme.cpp
    src/ui/mainwindow/qcmainwindow.cpp
    src/ui/mainwindow/qcnavigationtreedelegate.cpp
    src/ui/mainwindow/qclegacyfiltersupport.cpp
    src/ui/dialogs/qcaisettingsdialog.cpp
    src/ui/dialogs/qccreateimagesnippetdialog.cpp
    src/ui/dialogs/qccreatesessiondialog.cpp
    src/ui/dialogs/qccreatetextsnippetdialog.cpp
    src/ui/dialogs/qcquickcapturedialog.cpp
    src/ui/dialogs/qcsnippettagdialog.cpp
    src/ui/widgets/qcscreenshotoverlay.cpp
)

set(QTCLIP_TRANSLATIONS_TS
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/qtclip_zh_CN.ts
)
set(QTCLIP_TRANSLATIONS_QM_DIR ${CMAKE_CURRENT_BINARY_DIR}/i18n)
set(QTCLIP_TRANSLATIONS_QM
    ${QTCLIP_TRANSLATIONS_QM_DIR}/qtclip_zh_CN.qm
)
set(QTCLIP_LUPDATE_EXECUTABLE "${QTCLIP_QT_BIN_DIR}/lupdate.exe")
set(QTCLIP_LRELEASE_EXECUTABLE "${QTCLIP_QT_BIN_DIR}/lrelease.exe")

if (EXISTS "${QTCLIP_LUPDATE_EXECUTABLE}")
    add_custom_target(qtclip_lupdate
        COMMAND "${QTCLIP_LUPDATE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/src" -ts ${QTCLIP_TRANSLATIONS_TS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running lupdate for QtClip translations"
    )
endif()

if (EXISTS "${QTCLIP_LRELEASE_EXECUTABLE}")
    add_custom_command(
        OUTPUT ${QTCLIP_TRANSLATIONS_QM}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${QTCLIP_TRANSLATIONS_QM_DIR}
        COMMAND "${QTCLIP_LRELEASE_EXECUTABLE}" ${QTCLIP_TRANSLATIONS_TS} -qm ${QTCLIP_TRANSLATIONS_QM}
        DEPENDS ${QTCLIP_TRANSLATIONS_TS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling Qt translations to .qm"
    )
    add_custom_target(qtclip_translations ALL DEPENDS ${QTCLIP_TRANSLATIONS_QM})
endif()

add_executable(qtclipsmoke WIN32
    src/app/main.cpp
    ${QTCLIP_CORE_DATABASE_SOURCES}
    ${QTCLIP_DOMAIN_ENTITY_SOURCES}
    ${QTCLIP_REPOSITORY_SQLITE_SOURCES}
    ${QTCLIP_SERVICE_SOURCES}
    ${QTCLIP_UI_SOURCES}
)

set_target_properties(qtclipsmoke PROPERTIES
    VS_GLOBAL_VcpkgApplocalDeps false
)

target_include_directories(qtclipsmoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(qtclipsmoke PRIVATE
    Qt5::Concurrent
    Qt5::Core
    Qt5::Network
    Qt5::Sql
    Qt5::Widgets
    Shlwapi
)

if (TARGET qtclip_translations)
    add_dependencies(qtclipsmoke qtclip_translations)
endif()

if (EXISTS "${QTCLIP_WINDEPLOYQT_EXECUTABLE}")
    add_custom_command(TARGET qtclipsmoke POST_BUILD
        COMMAND "${QTCLIP_WINDEPLOYQT_EXECUTABLE}"
            --no-compiler-runtime
            --no-translations
            --no-angle
            --no-opengl-sw
            "$<$<CONFIG:Release>:$<TARGET_FILE:qtclipsmoke>>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${QTCLIP_VCPKG_BIN_DIR}/zlib1.dll"
            "$<TARGET_FILE_DIR:qtclipsmoke>/zlib1.dll"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${QTCLIP_VCPKG_BIN_DIR}/libssl-3-x64.dll"
            "$<TARGET_FILE_DIR:qtclipsmoke>/libssl-3-x64.dll"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${QTCLIP_VCPKG_BIN_DIR}/libcrypto-3-x64.dll"
            "$<TARGET_FILE_DIR:qtclipsmoke>/libcrypto-3-x64.dll"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${QTCLIP_VCPKG_BIN_DIR}"
            "$<TARGET_FILE_DIR:qtclipsmoke>"
        COMMENT "Deploying Qt runtime for qtclipsmoke."
    )
endif()









add_custom_command(TARGET qtclipsmoke POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:qtclipsmoke>/i18n"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QTCLIP_TRANSLATIONS_QM_DIR}/qtclip_zh_CN.qm"
        "$<TARGET_FILE_DIR:qtclipsmoke>/i18n/qtclip_zh_CN.qm"
    COMMENT "Copying QtClip translation files."
)
