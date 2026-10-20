cmake_minimum_required(VERSION 3.5.0)
project(ktouch_harmony)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(HARMONY_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
get_filename_component(KTOUCH_ROOT "${HARMONY_PROJECT_ROOT}/.." ABSOLUTE)

if (NOT IS_ABSOLUTE "${QT_PREFIX}")
    get_filename_component(QT_PREFIX "${HARMONY_PROJECT_ROOT}/${QT_PREFIX}" ABSOLUTE)
endif()

if (NOT EXISTS "${QT_PREFIX}/lib/cmake/Qt5/Qt5Config.cmake")
    message(FATAL_ERROR "QT_PREFIX is invalid: ${QT_PREFIX}")
endif()

list(PREPEND CMAKE_PREFIX_PATH ${QT_PREFIX})
list(APPEND CMAKE_FIND_ROOT_PATH ${QT_PREFIX})

set(QT_HARMONY_QML_MODULE_ROOTS
    QtGraphicalEffects
    QtQml
    QtQuick/Controls.2
    QtQuick/Layouts
    QtQuick/Templates.2
    QtQuick/Window.2
)

set(QT_HARMONY_QML_QRC "${CMAKE_CURRENT_BINARY_DIR}/qt_harmony_qml_modules.qrc")
set(QT_HARMONY_QML_PATCH_DIR "${CMAKE_CURRENT_BINARY_DIR}/qt_harmony_qml_patched")
file(REMOVE_RECURSE "${QT_HARMONY_QML_PATCH_DIR}")
file(MAKE_DIRECTORY "${QT_HARMONY_QML_PATCH_DIR}")
file(WRITE "${QT_HARMONY_QML_QRC}" "<RCC>\n    <qresource prefix=\"/qt-project.org/imports\">\n")

set(QT_HARMONY_QML_PLUGIN_FILES)
foreach(module_root IN LISTS QT_HARMONY_QML_MODULE_ROOTS)
    file(GLOB_RECURSE module_files CONFIGURE_DEPENDS "${QT_PREFIX}/qml/${module_root}/*")
    foreach(module_file IN LISTS module_files)
        if (IS_DIRECTORY "${module_file}")
            continue()
        endif()

        if (module_file MATCHES "\\.so$")
            list(APPEND QT_HARMONY_QML_PLUGIN_FILES "${module_file}")
        else()
            file(RELATIVE_PATH module_alias "${QT_PREFIX}/qml" "${module_file}")
            set(module_resource_file "${module_file}")

            if (module_file MATCHES "\\.qml$")
                set(module_resource_file "${QT_HARMONY_QML_PATCH_DIR}/${module_alias}")
                get_filename_component(module_resource_dir "${module_resource_file}" DIRECTORY)
                file(MAKE_DIRECTORY "${module_resource_dir}")
                file(READ "${module_file}" module_qml_contents)
                string(REPLACE
                    "font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal"
                    ""
                    module_qml_contents
                    "${module_qml_contents}"
                )
                string(REPLACE
                    "font.bold: true"
                    ""
                    module_qml_contents
                    "${module_qml_contents}"
                )
                string(REGEX REPLACE
                    "origin\\.x:[^\n]*\n[ \t]*origin\\.y:[^\n]*"
                    ""
                    module_qml_contents
                    "${module_qml_contents}"
                )
                file(WRITE "${module_resource_file}" "${module_qml_contents}")
            endif()

            file(APPEND "${QT_HARMONY_QML_QRC}" "        <file alias=\"${module_alias}\">${module_resource_file}</file>\n")
        endif()
    endforeach()
endforeach()

file(APPEND "${QT_HARMONY_QML_QRC}" "    </qresource>\n</RCC>\n")
list(REMOVE_DUPLICATES QT_HARMONY_QML_PLUGIN_FILES)

find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    OhExtras
    Qml
    Quick
    QuickControls2
    QuickTemplates2
    QuickWidgets
    Sql
    Svg
    Widgets
    Xml
)

set(KTOUCH_SOURCES
    ${KTOUCH_ROOT}/src/application.cpp
    ${KTOUCH_ROOT}/src/application.h
    ${KTOUCH_ROOT}/src/bindings/stringformatter.cpp
    ${KTOUCH_ROOT}/src/bindings/stringformatter.h
    ${KTOUCH_ROOT}/src/bindings/utils.cpp
    ${KTOUCH_ROOT}/src/bindings/utils.h
    ${KTOUCH_ROOT}/src/core/abstractkey.cpp
    ${KTOUCH_ROOT}/src/core/abstractkey.h
    ${KTOUCH_ROOT}/src/core/course.cpp
    ${KTOUCH_ROOT}/src/core/course.h
    ${KTOUCH_ROOT}/src/core/coursebase.cpp
    ${KTOUCH_ROOT}/src/core/coursebase.h
    ${KTOUCH_ROOT}/src/core/dataaccess.cpp
    ${KTOUCH_ROOT}/src/core/dataaccess.h
    ${KTOUCH_ROOT}/src/core/dataindex.cpp
    ${KTOUCH_ROOT}/src/core/dataindex.h
    ${KTOUCH_ROOT}/src/core/dbaccess.cpp
    ${KTOUCH_ROOT}/src/core/dbaccess.h
    ${KTOUCH_ROOT}/src/core/key.cpp
    ${KTOUCH_ROOT}/src/core/key.h
    ${KTOUCH_ROOT}/src/core/keyboardlayout.cpp
    ${KTOUCH_ROOT}/src/core/keyboardlayout.h
    ${KTOUCH_ROOT}/src/core/keyboardlayoutbase.cpp
    ${KTOUCH_ROOT}/src/core/keyboardlayoutbase.h
    ${KTOUCH_ROOT}/src/core/keychar.cpp
    ${KTOUCH_ROOT}/src/core/keychar.h
    ${KTOUCH_ROOT}/src/core/lesson.cpp
    ${KTOUCH_ROOT}/src/core/lesson.h
    ${KTOUCH_ROOT}/src/core/profile.cpp
    ${KTOUCH_ROOT}/src/core/profile.h
    ${KTOUCH_ROOT}/src/core/profiledataaccess.cpp
    ${KTOUCH_ROOT}/src/core/profiledataaccess.h
    ${KTOUCH_ROOT}/src/core/resource.cpp
    ${KTOUCH_ROOT}/src/core/resource.h
    ${KTOUCH_ROOT}/src/core/resourcedataaccess.cpp
    ${KTOUCH_ROOT}/src/core/resourcedataaccess.h
    ${KTOUCH_ROOT}/src/core/specialkey.cpp
    ${KTOUCH_ROOT}/src/core/specialkey.h
    ${KTOUCH_ROOT}/src/core/trainingstats.cpp
    ${KTOUCH_ROOT}/src/core/trainingstats.h
    ${KTOUCH_ROOT}/src/core/userdataaccess.cpp
    ${KTOUCH_ROOT}/src/core/userdataaccess.h
    ${KTOUCH_ROOT}/src/declarativeitems/griditem.cpp
    ${KTOUCH_ROOT}/src/declarativeitems/griditem.h
    ${KTOUCH_ROOT}/src/declarativeitems/kcolorschemeproxy.cpp
    ${KTOUCH_ROOT}/src/declarativeitems/kcolorschemeproxy.h
    ${KTOUCH_ROOT}/src/declarativeitems/lessonpainter.cpp
    ${KTOUCH_ROOT}/src/declarativeitems/lessonpainter.h
    ${KTOUCH_ROOT}/src/declarativeitems/lessontexthighlighteritem.cpp
    ${KTOUCH_ROOT}/src/declarativeitems/lessontexthighlighteritem.h
    ${KTOUCH_ROOT}/src/declarativeitems/preferencesproxy.cpp
    ${KTOUCH_ROOT}/src/declarativeitems/preferencesproxy.h
    ${KTOUCH_ROOT}/src/declarativeitems/scalebackgrounditem.cpp
    ${KTOUCH_ROOT}/src/declarativeitems/scalebackgrounditem.h
    ${KTOUCH_ROOT}/src/declarativeitems/traininglinecore.cpp
    ${KTOUCH_ROOT}/src/declarativeitems/traininglinecore.h
    ${KTOUCH_ROOT}/src/editor/lessontexthighlighter.cpp
    ${KTOUCH_ROOT}/src/editor/lessontexthighlighter.h
    ${KTOUCH_ROOT}/src/ktouchcontext.cpp
    ${KTOUCH_ROOT}/src/ktouchcontext.h
    ${KTOUCH_ROOT}/src/main.cpp
    ${KTOUCH_ROOT}/src/mainwindow.cpp
    ${KTOUCH_ROOT}/src/mainwindow.h
    ${KTOUCH_ROOT}/src/models/categorizedresourcesortfilterproxymodel.cpp
    ${KTOUCH_ROOT}/src/models/categorizedresourcesortfilterproxymodel.h
    ${KTOUCH_ROOT}/src/models/errorsmodel.cpp
    ${KTOUCH_ROOT}/src/models/errorsmodel.h
    ${KTOUCH_ROOT}/src/models/learningprogressmodel.cpp
    ${KTOUCH_ROOT}/src/models/learningprogressmodel.h
    ${KTOUCH_ROOT}/src/models/lessonmodel.cpp
    ${KTOUCH_ROOT}/src/models/lessonmodel.h
    ${KTOUCH_ROOT}/src/models/resourcemodel.cpp
    ${KTOUCH_ROOT}/src/models/resourcemodel.h
    ${KTOUCH_ROOT}/src/images/images.qrc
    ${KTOUCH_ROOT}/src/qml/qml.qrc
    ${QT_HARMONY_QML_QRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/openharmony_compat/ohosi18ncontext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/openharmony_compat/ohosi18ncontext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/openharmony_data/ktouch_ohos_data.qrc
)

add_library(entry SHARED ${KTOUCH_SOURCES})

target_include_directories(entry PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/openharmony_compat
    ${KTOUCH_ROOT}
    ${KTOUCH_ROOT}/src
    ${QT_PREFIX}/include/QtCore/5.15.12
    ${QT_PREFIX}/include/QtCore/5.15.12/QtCore
    ${QT_PREFIX}/include/QtQml/5.15.12
    ${QT_PREFIX}/include/QtQml/5.15.12/QtQml
)

target_compile_features(entry PRIVATE cxx_std_17)
target_compile_definitions(entry PRIVATE
    KTOUCH_HARMONY_MVP
    OPENHARMONY
    NAPI_DISABLE_CPP_EXCEPTIONS
)

set_target_properties(entry PROPERTIES
    BUILD_RPATH "$ORIGIN"
)

target_link_libraries(entry PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::OhExtras
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickControls2
    Qt5::QuickTemplates2
    Qt5::QuickWidgets
    Qt5::Sql
    Qt5::Svg
    Qt5::Widgets
    Qt5::Xml
    Qt5::QOpenHarmonyPlatformIntegrationPlugin
    Qt5::QGifPlugin
    Qt5::QICOPlugin
    Qt5::QJpegPlugin
    Qt5::QSvgPlugin
    Qt5::QSQLiteDriverPlugin
)

foreach(qml_plugin IN LISTS QT_HARMONY_QML_PLUGIN_FILES)
    add_custom_command(TARGET entry POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${qml_plugin}" "$<TARGET_FILE_DIR:entry>"
        VERBATIM
    )
endforeach()
