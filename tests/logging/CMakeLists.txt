# Logging Tests CMakeLists.txt

# Find required Qt components
find_package(Qt6 REQUIRED COMPONENTS Test Core)

# Create logging test executable
add_executable(logging_tests
    LoggingStubsTest.cpp
)

# Link against required libraries
target_link_libraries(logging_tests
    Qt6::Test
    Qt6::Core
    # Link against the main app library to access logging classes
    # Note: This assumes the main app is built as a library
    # If not, we may need to compile the logging sources directly
)

# Include directories
target_include_directories(logging_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/tests
)

# Add test to CTest
add_test(NAME LoggingStubsTest COMMAND logging_tests)

# Set test properties
set_tests_properties(LoggingStubsTest PROPERTIES
    TIMEOUT 30
    LABELS "unit;logging"
)

# Enable Qt's automatic MOC, UIC, and RCC processing
set_target_properties(logging_tests PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Compiler settings
target_compile_features(logging_tests PRIVATE cxx_std_20)

# Add compiler flags for testing
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(logging_tests PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Windows-specific settings
if(WIN32)
    # Ensure proper Windows subsystem
    set_target_properties(logging_tests PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# Add custom target for running logging tests
add_custom_target(run_logging_tests
    COMMAND logging_tests
    DEPENDS logging_tests
    COMMENT "Running logging tests"
)
