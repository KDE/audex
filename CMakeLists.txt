# SPDX-FileCopyrightText: 2007-2026 Marco Nelles
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16.0)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "26")
set(RELEASE_SERVICE_VERSION_MINOR "11")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION
    "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}"
)

project(Audex VERSION ${RELEASE_SERVICE_VERSION})

set(KF6_MIN_VERSION "6.5.0")
set(QT_MIN_VERSION "6.5.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(FeatureSummary)
include(KDEClangFormat)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets Concurrent
                                                       Network DBus)
find_package(
  KF6 ${KF6_MIN_VERSION} REQUIRED
  COMPONENTS Config
             Completion
             CoreAddons
             Crash
             I18n
             IconThemes
             KIO
             Solid
             TextWidgets
             WidgetsAddons
             XmlGui)

find_package(TagLib 1.12 REQUIRED)
set_package_properties(
  TagLib PROPERTIES
  DESCRIPTION "Audio meta-data library"
  URL "https://taglib.org"
  TYPE REQUIRED
  PURPOSE "Writing tags into the ripped audio files.")

if(TARGET TagLib::tag)
  set(AUDEX_TAGLIB_TARGET TagLib::tag)
elseif(TARGET TagLib::TagLib)
  set(AUDEX_TAGLIB_TARGET TagLib::TagLib)
else()
  message(
    FATAL_ERROR
      "TagLib found, but no usable CMake target (TagLib::tag / TagLib::TagLib)."
  )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050e00)

ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX AUDEX VERSION_HEADER
                  ${CMAKE_CURRENT_BINARY_DIR}/audex-version.h)

remove_definitions(
  -DQT_NO_CAST_FROM_ASCII -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY -DQT_USE_QSTRINGBUILDER -DQT_NO_FOREACH)

# ---------------------------------------------------------------------------
# extraction engine (pure Qt6, no KDE frameworks): CDDA reading, metadata,
# encoding, online providers, disc model, rip job
# ---------------------------------------------------------------------------
set(audex_engine_SRCS
    core/cacheprobe.cpp
    core/cdrmanufacturer.cpp
    core/cdtext.cpp
    core/checksums.cpp
    core/ctdbparity.cpp
    core/discid.cpp
    core/drivefeatures.cpp
    core/mmcparse.cpp
    core/readplan.cpp
    core/ripengine.cpp
    core/ripreport.cpp
    core/cdg.cpp
    core/subchannel.cpp
    core/hdcd.cpp
    core/toc.cpp
    device/mmc.cpp
    device/mmcsectorreader.cpp
    device/scsidevice.cpp
    sim/simulatedsectorreader.cpp
    metadata/cdinfo.cpp
    metadata/cdtextmetadata.cpp
    metadata/fields.cpp
    metadata/lookup.cpp
    metadata/metadata.cpp
    metadata/musicbrainz.cpp
    encoding/encodedoutputs.cpp
    encoding/registry.cpp
    encoding/tagwriter.cpp
    online/accuraterip.cpp
    online/blockingfetch.cpp
    online/ctdb.cpp
    online/coverartarchive.cpp
    online/coverartfetcher.cpp
    online/musicbrainzprovider.cpp
    models/cdinfomodel.cpp
    utils/ripjob.cpp
    utils/imagerepair.cpp
    utils/discsource.cpp
    utils/encodercommand.cpp
    utils/scheme.cpp)

add_library(audexengine STATIC ${audex_engine_SRCS})
target_include_directories(audexengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(audexengine PUBLIC Qt6::Core Qt6::Gui Qt6::Network
                                         ${AUDEX_TAGLIB_TARGET})
target_compile_definitions(
  audexengine
  PRIVATE
    AUDEX_ENCODER_PLUGIN_INSTALL_DIR="${KDE_INSTALL_FULL_PLUGINDIR}/audex_encoders"
)
set_target_properties(audexengine PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# encoder plugins (loaded by the engine at runtime, all optional)
# ---------------------------------------------------------------------------
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(FLAC QUIET IMPORTED_TARGET flac)
  pkg_check_modules(OPUSENC QUIET IMPORTED_TARGET opusenc)
endif()
# fallbacks for systems without pkg-config metadata (header + library is enough)
if(NOT FLAC_FOUND)
  find_path(FLAC_INCLUDE_DIR FLAC/stream_encoder.h)
  find_library(FLAC_LIBRARY NAMES FLAC)
  if(FLAC_INCLUDE_DIR AND FLAC_LIBRARY)
    set(FLAC_FOUND TRUE)
  endif()
endif()
if(NOT OPUSENC_FOUND)
  find_path(OPUSENC_INCLUDE_DIR opusenc.h PATH_SUFFIXES opus)
  find_library(OPUSENC_LIBRARY NAMES opusenc)
  if(OPUSENC_INCLUDE_DIR AND OPUSENC_LIBRARY)
    set(OPUSENC_FOUND TRUE)
  endif()
endif()
find_path(LAME_INCLUDE_DIR lame/lame.h)
find_library(LAME_LIBRARY NAMES mp3lame)
if(LAME_INCLUDE_DIR AND LAME_LIBRARY)
  set(LAME_FOUND TRUE)
endif()

set(AUDEX_ENCODER_PLUGIN_BUILD_DIR ${CMAKE_BINARY_DIR}/plugins/audex_encoders)

if(FLAC_FOUND)
  add_library(audex_flac MODULE plugins/flac/flacencoder.cpp)
  target_include_directories(audex_flac PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  if(TARGET PkgConfig::FLAC)
    target_link_libraries(audex_flac PRIVATE Qt6::Core Qt6::Gui PkgConfig::FLAC)
  else()
    target_include_directories(audex_flac PRIVATE ${FLAC_INCLUDE_DIR})
    target_link_libraries(audex_flac PRIVATE Qt6::Core Qt6::Gui ${FLAC_LIBRARY})
  endif()
  set_target_properties(audex_flac PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                              ${AUDEX_ENCODER_PLUGIN_BUILD_DIR})
  install(TARGETS audex_flac
          DESTINATION ${KDE_INSTALL_PLUGINDIR}/audex_encoders)
else()
  message(
    WARNING
      "libFLAC not found: the FLAC encoder plugin will NOT be built. Install the development package (e.g. 'libflac-dev') and re-run cmake. Without the plugin, FLAC is not available."
  )
endif()

if(LAME_FOUND)
  add_library(audex_mp3 MODULE plugins/mp3/lameencoder.cpp)
  target_include_directories(audex_mp3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                               ${LAME_INCLUDE_DIR})
  target_link_libraries(audex_mp3 PRIVATE Qt6::Core Qt6::Gui ${LAME_LIBRARY})
  set_target_properties(audex_mp3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                             ${AUDEX_ENCODER_PLUGIN_BUILD_DIR})
  install(TARGETS audex_mp3 DESTINATION ${KDE_INSTALL_PLUGINDIR}/audex_encoders)
else()
  message(
    WARNING
      "LAME not found: the MP3 encoder plugin will NOT be built. Install the development package (e.g. 'libmp3lame-dev') and re-run cmake. Without the plugin, MP3 is not available."
  )
endif()

if(OPUSENC_FOUND)
  add_library(audex_opus MODULE plugins/opus/opusencoder.cpp)
  target_include_directories(audex_opus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  if(TARGET PkgConfig::OPUSENC)
    target_link_libraries(audex_opus PRIVATE Qt6::Core Qt6::Gui
                                             PkgConfig::OPUSENC)
  else()
    target_include_directories(audex_opus PRIVATE ${OPUSENC_INCLUDE_DIR})
    target_link_libraries(audex_opus PRIVATE Qt6::Core Qt6::Gui
                                             ${OPUSENC_LIBRARY})
  endif()
  set_target_properties(audex_opus PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                              ${AUDEX_ENCODER_PLUGIN_BUILD_DIR})
  install(TARGETS audex_opus
          DESTINATION ${KDE_INSTALL_PLUGINDIR}/audex_encoders)
else()
  message(
    WARNING
      "libopusenc not found: the Opus encoder plugin will NOT be built. Install the development package (e.g. 'libopusenc-dev', Arch: 'libopusenc') and re-run cmake. Without the plugin, Opus is not available."
  )
endif()

message(
  STATUS
    "Encoder plugins to build: FLAC=${FLAC_FOUND} MP3=${LAME_FOUND} OPUS=${OPUSENC_FOUND}"
)

# ---------------------------------------------------------------------------
# application
# ---------------------------------------------------------------------------
set(audex_SRCS
    main.cpp
    mainwindow.cpp
    models/profilemodel.cpp
    utils/schemeparser.cpp
    utils/playlist.cpp
    utils/encoderassistant.cpp
    utils/parameters.cpp
    utils/cuesheetwriter.cpp
    utils/devicesettings.cpp
    utils/disccontroller.cpp
    utils/riprequestbuilder.cpp
    utils/genres.cpp
    widgets/cddaheaderwidget.cpp
    widgets/generalsettingswidget.cpp
    widgets/devicewidget.cpp
    widgets/profilewidget.cpp
    widgets/lamewidget.cpp
    widgets/opusencwidget.cpp
    widgets/flacwidget.cpp
    widgets/wavewidget.cpp
    widgets/customwidget.cpp
    widgets/schemeedit.cpp
    widgets/discplaceholder.cpp
    widgets/discmapwidget.cpp
    dialogs/cddaheaderdatadialog.cpp
    dialogs/metadatacandidatedialog.cpp
    dialogs/coverchooserdialog.cpp
    dialogs/profiledatadialog.cpp
    dialogs/profiledatacoverdialog.cpp
    dialogs/profiledataplaylistdialog.cpp
    dialogs/profiledatalogfiledialog.cpp
    dialogs/profiledatahookdialog.cpp
    dialogs/driveassistantdialog.cpp
    dialogs/extractingprogressdialog.cpp
    dialogs/logviewdialog.cpp
    dialogs/errordialog.cpp
    dialogs/settingsdialog.cpp)

ki18n_wrap_ui(
  audex_SRCS
  widgets/generalsettingswidgetUI.ui
  widgets/devicewidgetUI.ui
  widgets/profilewidgetUI.ui
  widgets/lamewidgetUI.ui
  widgets/opusencwidgetUI.ui
  widgets/flacwidgetUI.ui
  widgets/wavewidgetUI.ui
  widgets/customwidgetUI.ui
  dialogs/cddaheaderdatawidgetUI.ui
  dialogs/profiledatawidgetUI.ui
  dialogs/profiledatacoverwidgetUI.ui
  dialogs/profiledataplaylistwidgetUI.ui
  dialogs/profiledatalogfilewidgetUI.ui
  dialogs/profiledatahookwidgetUI.ui
  dialogs/driveassistantwidgetUI.ui
  dialogs/extractingprogresswidgetUI.ui
  dialogs/logviewwidgetUI.ui)

kconfig_add_kcfg_files(audex_SRCS preferences.kcfgc)
qt_add_resources(audex_SRCS audex.qrc)

add_executable(audex ${audex_SRCS})
target_link_libraries(
  audex
  audexengine
  Qt6::Widgets
  Qt6::Concurrent
  Qt6::Network
  Qt6::DBus
  KF6::Completion
  KF6::CoreAddons
  KF6::Crash
  KF6::I18n
  KF6::IconThemes
  KF6::IconWidgets
  KF6::KIOWidgets
  KF6::Solid
  KF6::TextWidgets
  KF6::WidgetsAddons
  KF6::XmlGui)

ecm_add_app_icon(
  audex
  ICONS
  icons/sc-apps-org.kde.audex.svg
  icons/16-apps-org.kde.audex.png
  icons/32-apps-org.kde.audex.png
  icons/48-apps-org.kde.audex.png
  icons/64-apps-org.kde.audex.png
  icons/128-apps-org.kde.audex.png)

install(TARGETS audex DESTINATION ${KDE_INSTALL_BINDIR})
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.audex.desktop)
  install(PROGRAMS org.kde.audex.desktop DESTINATION ${KDE_INSTALL_APPDIR})
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.audex.metainfo.xml)
  install(FILES org.kde.audex.metainfo.xml
          DESTINATION ${KDE_INSTALL_METAINFODIR})
endif()
install(FILES audex-rip-audiocd.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions)
install(FILES org.kde.audex.svg
        DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

add_subdirectory(images)

# Engine test suite. The tests use the deterministic simulated CD drive, so no
# physical optical drive is required.
enable_testing()
add_subdirectory(tests)

# Install translations (the po directory is maintained by KDE l10n and may not
# be present in every checkout)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/po)
  ki18n_install(po)
endif()

install(FILES audex.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

# example commands for the custom encoder (users can add their own in
# ~/.local/share/audex/encoderpresets.json)
install(FILES encoderpresets.json DESTINATION ${KDE_INSTALL_DATADIR}/audex)

# example genres (users can add their own in ~/.local/share/audex/genres.json)
install(FILES genres.json DESTINATION ${KDE_INSTALL_DATADIR}/audex)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES)
