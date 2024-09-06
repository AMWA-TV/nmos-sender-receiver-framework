add_subdirectory(lib)

if (BISECT_CPP_CORE_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
