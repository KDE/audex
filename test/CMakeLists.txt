function(add_unit_tests)
    foreach(test ${ARGN})
        add_executable(${test} ${test}.cpp)
        add_test( ${test} ${test} )
        target_link_libraries(${test} Qt5::Test KF5::Solid)
    endforeach()
endfunction()

add_unit_tests(
    devicestest)
