cmake_minimum_required(VERSION 3.16.0)

project(Audex VERSION "0.96")

set(KF5_MIN_VERSION "5.100.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMAddTests)
include(FeatureSummary)
include(KDEClangFormat)

find_package(Qt6 6.5 REQUIRED COMPONENTS
  Widgets
  Qml
  Core5Compat
)
find_package(KF6 ${KF_MIN_VER} REQUIRED COMPONENTS
  Config
  Completion
  CoreAddons
  Crash
  I18n
  IconThemes
  KCMUtils
  KIO
  Solid
  TextWidgets
  WidgetsAddons
  XmlGui
)
find_package(KCddb6 5.1)
set_package_properties(KCddb6 PROPERTIES
  DESCRIPTION "CDDB library"
  URL "https://commits.kde.org/libkcddb"
  TYPE REQUIRED
  PURPOSE "libkcddb is used to retrieve audio CD meta data from the internet.")
find_package(Cdparanoia REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}
                    ${CDPARANOIA_INCLUDE_DIR})

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050e00)

remove_definitions(
  -DQT_NO_CAST_FROM_ASCII -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY -DQT_USE_QSTRINGBUILDER -DQT_NO_FOREACH)

set(CMAKE_C_FLAGS "${AUDEX_ID3_CFLAGS} ${CMAKE_C_FLAGS}")

set(audex_SRCS
    main.cpp
    mainwindow.cpp
    models/cddamodel.cpp
    models/profilemodel.cpp
    utils/pid.cpp
    utils/patternparser.cpp
    utils/playlist.cpp
    utils/wavefilewriter.cpp
    utils/cddaparanoia.cpp
    utils/cddadevices.cpp
    utils/encoderwrapper.cpp
    utils/cddaextractthread.cpp
    utils/upload.cpp
    utils/encoderassistant.cpp
    utils/hashlist.cpp
    utils/3rdparty/crc32.cpp
    utils/parameters.cpp
    utils/cachedimage.cpp
    utils/cuesheetwriter.cpp
    utils/tmpdir.cpp
    utils/discidcalculator.cpp
    widgets/cddaheaderwidget.cpp
    widgets/generalsettingswidget.cpp
    widgets/devicewidget.cpp
    widgets/profilewidget.cpp
    widgets/remoteserversettingswidget.cpp
    widgets/lamewidget.cpp
    widgets/oggencwidget.cpp
    widgets/opusencwidget.cpp
    widgets/flacwidget.cpp
    widgets/faacwidget.cpp
    widgets/wavewidget.cpp
    widgets/customwidget.cpp
    dialogs/cddaheaderdatadialog.cpp
    dialogs/profiledatadialog.cpp
    dialogs/profiledatacoverdialog.cpp
    dialogs/profiledataplaylistdialog.cpp
    dialogs/profiledatainfodialog.cpp
    dialogs/profiledatahashlistdialog.cpp
    dialogs/profiledatacuesheetdialog.cpp
    dialogs/profiledatasinglefiledialog.cpp
    dialogs/patternwizarddialog.cpp
    dialogs/commandwizarddialog.cpp
    dialogs/simplepatternwizarddialog.cpp
    dialogs/extractingprogressdialog.cpp
    dialogs/protocoldialog.cpp
    dialogs/errordialog.cpp
    core/audex.cpp)

ki18n_wrap_ui(
  audex_SRCS
  widgets/generalsettingswidgetUI.ui
  widgets/devicewidgetUI.ui
  widgets/profilewidgetUI.ui
  widgets/remoteserversettingswidgetUI.ui
  widgets/lamewidgetUI.ui
  widgets/oggencwidgetUI.ui
  widgets/opusencwidgetUI.ui
  widgets/flacwidgetUI.ui
  widgets/faacwidgetUI.ui
  widgets/wavewidgetUI.ui
  widgets/customwidgetUI.ui
  dialogs/cddaheaderdatawidgetUI.ui
  dialogs/profiledatawidgetUI.ui
  dialogs/profiledatacoverwidgetUI.ui
  dialogs/profiledataplaylistwidgetUI.ui
  dialogs/profiledatainfowidgetUI.ui
  dialogs/profiledatahashlistwidgetUI.ui
  dialogs/profiledatacuesheetwidgetUI.ui
  dialogs/profiledatasinglefilewidgetUI.ui
  dialogs/patternwizardwidgetUI.ui
  dialogs/commandwizardwidgetUI.ui
  dialogs/simplepatternwizardwidgetUI.ui
  dialogs/extractingprogresswidgetUI.ui
  dialogs/protocolwidgetUI.ui)

kconfig_add_kcfg_files(audex_SRCS preferences.kcfgc)
qt_add_resources(audex_SRCS audex.qrc)

add_executable(audex ${audex_SRCS})
target_link_libraries(audex
    Qt6::Widgets
    Qt6::Qml
    Qt6::Core5Compat
    KF6::Completion
    KF6::CoreAddons
    KF6::Crash
    KF6::I18n
    KF6::IconThemes
    KF6::IconWidgets
    KF6::KCMUtils
    KF6::KIOWidgets
    KF6::Solid
    KF6::TextWidgets
    KF6::WidgetsAddons
    KF6::XmlGui
    KCddb6
    ${CDPARANOIA_LIBRARIES}
)

install(TARGETS audex DESTINATION ${KDE_INSTALL_BINDIR})
install(PROGRAMS org.kde.audex.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.audex.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES audex-rip-audiocd.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions)

add_subdirectory(icons)
add_subdirectory(images)

if(BUILD_TESTING)
    find_package(Qt6Test REQUIRED)
    add_subdirectory(test)
endif()

# Install translations
ki18n_install(po)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES)
