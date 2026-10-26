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
  test_ripstatus.cpp)

target_include_directories(audex_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(audex_tests PRIVATE audexengine)
# the AIFF plugin needs no codec library, so it is always there to test
add_dependencies(audex_tests audex_aiff)
target_compile_definitions(audex_tests PRIVATE AUDEX_TEST_PLUGIN_DIR="${AUDEX_ENCODER_PLUGIN_BUILD_DIR}")

add_test(NAME audex_tests COMMAND audex_tests)

# audex-cli with the simulated drive
add_test(NAME audex_cli_rip COMMAND ${CMAKE_COMMAND} -DCLI=$<TARGET_FILE:audex-cli> -DWORK=${CMAKE_CURRENT_BINARY_DIR}/cli_rip
                                    -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_rip.cmake)

# Exercise the Qt engine translator with the same KDE gettext catalog as the GUI.
find_package(Gettext REQUIRED)

set(audex_i18n_test_catalogs)
foreach(lang de fr)
  set(po "${CMAKE_CURRENT_SOURCE_DIR}/i18n/${lang}/audex.po")
  set(mo "${CMAKE_CURRENT_BINARY_DIR}/i18n/${lang}/LC_MESSAGES/audex.mo")
  get_filename_component(mo_dir "${mo}" DIRECTORY)
  add_custom_command(
    OUTPUT "${mo}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${mo_dir}"
    COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}" -o "${mo}" "${po}"
    DEPENDS "${po}"
    VERBATIM)
  list(APPEND audex_i18n_test_catalogs ${mo})
endforeach()

add_custom_target(audex_i18n_catalogs DEPENDS ${audex_i18n_test_catalogs})
add_executable(audex_i18n_tests test_main.cpp test_i18n.cpp ../utils/enginetranslator.cpp)
add_dependencies(audex_i18n_tests audex_i18n_catalogs)
target_include_directories(audex_i18n_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_definitions(audex_i18n_tests PRIVATE AUDEX_TEST_LOCALE_DIR="${CMAKE_CURRENT_BINARY_DIR}/i18n")
target_link_libraries(audex_i18n_tests PRIVATE Qt6::Core KF6::I18n)
add_test(NAME audex_i18n_tests COMMAND audex_i18n_tests)
