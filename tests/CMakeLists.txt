include(GkfsPythonTesting)

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND}
            --force-new-ctest-process
            --verbose
            --output-on-failure
)

# integration tests
add_subdirectory(integration)

# unit tests
add_subdirectory(unit)
