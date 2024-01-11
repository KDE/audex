cmake_minimum_required(VERSION 3.16.0)

project(Audex VERSION "0.96.2")

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

find_package(Qt5 5.14 REQUIRED COMPONENTS Script Widgets X11Extras)
find_package(
  KF5 ${KF5_MIN_VER} REQUIRED
  COMPONENTS Config
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
             XmlGui)
find_package(KF5Cddb 5.1)
set_package_properties(
  KF5Cddb PROPERTIES
  DESCRIPTION "KF5 branch for CDDB library"
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
    utils/schemeparser.cpp
    utils/playlist.cpp
    utils/wavefilewriter.cpp
    utils/cddaparanoia.cpp
    utils/cddadevices.cpp
    utils/encoderwrapper.cpp
    utils/cddaextractthread.cpp
    utils/upload.cpp
    utils/encoderassistant.cpp
    utils/hashlist.cpp
    utils/crc32hash.cpp
    utils/parameters.cpp
    utils/cuesheetwriter.cpp
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
    dialogs/schemewizarddialog.cpp
    dialogs/commandwizarddialog.cpp
    dialogs/filenameschemewizarddialog.cpp
    dialogs/extractingprogressdialog.cpp
    dialogs/protocolviewdialog.cpp
    dialogs/textviewdialog.cpp
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
  dialogs/schemewizardwidgetUI.ui
  dialogs/commandwizardwidgetUI.ui
  dialogs/filenameschemewizardwidgetUI.ui
  dialogs/extractingprogresswidgetUI.ui
  dialogs/protocolviewwidgetUI.ui
  dialogs/textviewwidgetUI.ui)

kconfig_add_kcfg_files(audex_SRCS preferences.kcfgc)
qt5_add_resources(audex_SRCS audex.qrc)

add_executable(audex ${audex_SRCS})
target_link_libraries(
  audex
  Qt5::Script
  Qt5::Widgets
  Qt5::X11Extras
  KF5::Completion
  KF5::CoreAddons
  KF5::Crash
  KF5::I18n
  KF5::IconThemes
  KF5::KCMUtils
  KF5::KIOWidgets
  KF5::Solid
  KF5::TextWidgets
  KF5::WidgetsAddons
  KF5::XmlGui
  KF5::Cddb
  ${CDPARANOIA_LIBRARIES})

install(TARGETS audex DESTINATION ${KDE_INSTALL_BINDIR})
install(PROGRAMS org.kde.audex.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.audex.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES audex-rip-audiocd.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions)

add_subdirectory(icons)
add_subdirectory(images)

if(BUILD_TESTING)
  find_package(Qt5Test REQUIRED)
  add_subdirectory(test)
endif()

# Install translations
ki18n_install(po)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES)
