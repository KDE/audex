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
  test_preemphasis.cpp
  test_drivefeatures.cpp
  test_cacheprobe.cpp
  test_coverartarchive.cpp
  test_cdg.cpp
  test_encodercommand.cpp)

target_include_directories(audex_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(audex_tests PRIVATE audexengine)

add_test(NAME audex_tests COMMAND audex_tests)
