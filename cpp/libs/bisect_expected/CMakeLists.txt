add_subdirectory(lib)

option(BISECT_EXPECTED_ENABLE_TESTS "Enable tests" ON)

if (BISECT_EXPECTED_ENABLE_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

