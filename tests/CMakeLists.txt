# SPDX-FileCopyrightText: 2007-2026 Marco Nelles
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Keep the engine tests independent of Qt Test.  The production engine already
# depends on Qt6::Core; the test runner only uses ordinary C++/QtCore APIs.
add_executable(
  audex_tests
  test_main.cpp
  test_framework.h
  test_readplan.cpp
  test_ripengine.cpp
  test_subchannel.cpp
  test_mmcparse.cpp
  test_checksums.cpp
  test_ctdb.cpp
  test_ctdbparity.cpp
  test_imagerepair.cpp
  test_fileops.cpp
  test_encodedoutputs.cpp
  test_preemphasis.cpp
  test_drivefeatures.cpp
  test_cacheprobe.cpp
  test_coverartarchive.cpp
  test_gnudb.cpp
  test_responsecache.cpp
  test_drivelock.cpp
  test_cdg.cpp
  test_encodercommand.cpp
  test_scheme.cpp
  test_cover.cpp
  test_tagwriter.cpp
  test_cuesheet.cpp
  test_cdinfo.cpp
  test_aiff.cpp
  test_ripreport.cpp
  test_ripverify.cpp
  test_ripstatus.cpp
  test_text.cpp)

target_include_directories(audex_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(audex_tests PRIVATE audexengine)
# the AIFF plugin needs no codec library, so it is always there to test
add_dependencies(audex_tests audex_aiff)
target_compile_definitions(audex_tests PRIVATE AUDEX_TEST_PLUGIN_DIR="${AUDEX_ENCODER_PLUGIN_BUILD_DIR}")

add_test(NAME audex_tests COMMAND audex_tests)

# audex-cli with the simulated drive
add_test(NAME audex_cli_rip COMMAND ${CMAKE_COMMAND} -DCLI=$<TARGET_FILE:audex-cli> -DWORK=${CMAKE_CURRENT_BINARY_DIR}/cli_rip
                                    -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_rip.cmake)

# The Qt catalog (engine and plugins) as installed and loaded by the ECM qm
# loader. The language comes from LANGUAGE, as with the application language of
# KXmlGui; LANG=C.UTF-8 keeps Qt from ignoring it.
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS LinguistTools)

set(audex_i18n_share "${CMAKE_CURRENT_BINARY_DIR}/i18n")
set(audex_i18n_test_catalogs)
foreach(lang de fr en)
  set(po "${CMAKE_CURRENT_SOURCE_DIR}/i18n/${lang}/audex_qt.po")
  set(ts_dir "${CMAKE_CURRENT_BINARY_DIR}/i18n-ts/${lang}")
  set(qm_dir "${audex_i18n_share}/locale/${lang}/LC_MESSAGES")
  # the same steps as ecm_install_po_files_as_qm()
  add_custom_command(
    OUTPUT "${qm_dir}/audex_qt.qm"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ts_dir}" "${qm_dir}"
    COMMAND Qt6::lconvert -i "${po}" -o "${ts_dir}/audex_qt.ts" -target-language ${lang}
    COMMAND Qt6::lrelease -removeidentical -nounfinished -silent "${ts_dir}/audex_qt.ts" -qm "${qm_dir}/audex_qt.qm"
    DEPENDS "${po}"
    VERBATIM)
  list(APPEND audex_i18n_test_catalogs "${qm_dir}/audex_qt.qm")
endforeach()

add_custom_target(audex_i18n_catalogs DEPENDS ${audex_i18n_test_catalogs})
add_executable(audex_i18n_tests test_main.cpp test_i18n.cpp)
ecm_create_qm_loader(audex_i18n_tests audex_qt)
add_dependencies(audex_i18n_tests audex_i18n_catalogs)
target_include_directories(audex_i18n_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(audex_i18n_tests PRIVATE Qt6::Core)
foreach(lang de fr en)
  add_test(NAME audex_i18n_tests_${lang} COMMAND audex_i18n_tests)
  set_tests_properties(
    audex_i18n_tests_${lang}
    PROPERTIES
      ENVIRONMENT
      "XDG_DATA_HOME=${audex_i18n_share};XDG_DATA_DIRS=${audex_i18n_share};LC_ALL=;LC_MESSAGES=;LANG=C.UTF-8;LANGUAGE=${lang}"
  )
endforeach()

# GUI playlist generation uses KDE I18n and is tested separately from the engine.
add_executable(audex_playlist_tests test_main.cpp test_playlist.cpp ../utils/playlist.cpp)
target_include_directories(audex_playlist_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(audex_playlist_tests PRIVATE Qt6::Core KF6::I18n)
add_test(NAME audex_playlist_tests COMMAND audex_playlist_tests)
