# SPDX-FileCopyrightText: 2026 Marco Nelles
#
# SPDX-License-Identifier: GPL-3.0-or-later

# End-to-end test of audex-cli with the simulated drive (no network):
#   cmake -DCLI=<audex-cli> -DWORK=<empty folder> -P cli_rip.cmake

file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})
set(ENV{XDG_CONFIG_HOME} ${WORK}/config) # no saved offsets

function(run expected)
  execute_process(COMMAND ${CLI} ${ARGN} RESULT_VARIABLE code OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT code EQUAL expected)
    message(FATAL_ERROR "audex-cli ${ARGN}: exit ${code}, expected ${expected}\n${out}\n${err}")
  endif()
  set(output "${out}${err}" PARENT_SCOPE)
endfunction()

function(expect_files folder)
  file(GLOB found RELATIVE ${folder} ${folder}/*)
  list(SORT found)
  set(wanted ${ARGN})
  list(SORT wanted)
  if(NOT "${found}" STREQUAL "${wanted}")
    message(FATAL_ERROR "${folder}: ${found}, expected ${wanted}")
  endif()
endfunction()

run(0 info -d sim:demo)
if(NOT output MATCHES "Simulated drive" OR NOT output MATCHES "HDCD: +no" OR NOT output MATCHES "CD\\+G: +no")
  message(FATAL_ERROR "info: ${output}")
endif()
run(0 info -d sim:demo --toc-only)
if(output MATCHES "HDCD:")
  message(FATAL_ERROR "info --toc-only: ${output}")
endif()

# no read offset known for the drive
run(64 rip -d sim:demo -o ${WORK}/all -q)

# the whole disc, the hidden track included (the demo one is not silent)
run(2 rip -d sim:demo -o ${WORK}/all -q --offset 6 --no-accuraterip)
expect_files(${WORK}/all 00.wav 01.wav 02.wav 03.wav 04.wav disc.cue disc.log)
if(NOT output MATCHES "WARNING: AccurateRip is turned off")
  message(FATAL_ERROR "no warning: ${output}")
endif()
file(READ ${WORK}/all/disc.log log)
if(NOT log MATCHES "NOT PERFORMED: turned off with --no-accuraterip")
  message(FATAL_ERROR "the log does not tell that AccurateRip was turned off")
endif()
file(READ ${WORK}/all/disc.cue cue)
if(NOT cue MATCHES "FILE \"01.wav\" WAVE" OR NOT cue MATCHES "INDEX 00")
  message(FATAL_ERROR "cue sheet: ${cue}")
endif()

# existing files are kept
run(3 rip -d sim:demo -o ${WORK}/all -q --offset 6 --no-accuraterip)

# the files match the log
run(0 verify --offline --quiet ${WORK}/all/disc.log)

# a track list
run(2 rip -d sim:demo -o ${WORK}/some -q --offset 6 --no-accuraterip -t 1,3-)
expect_files(${WORK}/some 01.wav 03.wav 04.wav disc.cue disc.log)
run(64 rip -d sim:demo -o ${WORK}/none -q --offset 6 --no-accuraterip -t 7)

# CD+G asked for, the demo disc has none
run(2 rip -d sim:demo -o ${WORK}/cdg -q --offset 6 --no-accuraterip -t 1 --cdg)
expect_files(${WORK}/cdg 01.wav disc.cue disc.log)
if(NOT output MATCHES "no graphics on the disc")
  message(FATAL_ERROR "--cdg: ${output}")
endif()
