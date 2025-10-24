cmake_minimum_required(VERSION 3.15)
project(status-rollup VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include FetchContent for dependency management
include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

# Fetch CGraph
FetchContent_Declare(
    CGraph
    GIT_REPOSITORY https://github.com/ChunelFeng/CGraph.git
    GIT_TAG v3.1.2
)

# Disable CGraph examples and tutorials to speed up build
set(CGRAPH_BUILD_TUTORIALS OFF CACHE BOOL "" FORCE)
set(CGRAPH_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CGRAPH_BUILD_FUNCTIONAL_TESTS OFF CACHE BOOL "" FORCE)
set(CGRAPH_BUILD_PERFORMANCE_TESTS OFF CACHE BOOL "" FORCE)

# Enable position-independent code for all targets (required for Python bindings)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Make dependencies available
FetchContent_MakeAvailable(nlohmann_json CGraph)

# Disable "warnings as errors" for CGraph to avoid third-party code breaking builds
if(TARGET CGraph)
    if(MSVC)
        target_compile_options(CGraph PRIVATE /WX-)
    else()
        target_compile_options(CGraph PRIVATE -Wno-error)
    endif()
endif()

# Get CGraph source directory for includes
FetchContent_GetProperties(CGraph)
if(NOT cgraph_POPULATED)
    FetchContent_Populate(CGraph)
endif()

# Create library
add_library(status_rollup_lib
    src/rollup_rule.cpp
    src/status_node.cpp
    src/status_tree.cpp
)

# Enable position-independent code for the library (required for shared libraries)
set_target_properties(status_rollup_lib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Set library include directories
target_include_directories(status_rollup_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    SYSTEM PRIVATE
        ${cgraph_SOURCE_DIR}/src
)

# Link library dependencies (mark CGraph as SYSTEM to suppress its warnings)
target_link_libraries(status_rollup_lib
    PUBLIC
        nlohmann_json::nlohmann_json
    PRIVATE
        CGraph
)

# Mark CGraph as a system library to suppress warnings
set_target_properties(CGraph PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:CGraph,INTERFACE_INCLUDE_DIRECTORIES>
)

# Create example executable
add_executable(rollup examples/basic_example.cpp)

# Link executable to library
target_link_libraries(rollup
    PRIVATE
        status_rollup_lib
)

# Set strict compiler warnings for modern C++
if(MSVC)
    set(WARNING_FLAGS
        /W4
        /WX          # Treat warnings as errors
        /permissive- # Enforce standards conformance
        /wd4068      # Disable unknown pragma warning (CGraph uses GCC pragmas)
        /wd4996      # Disable deprecated function warning (CGraph uses localtime)
        /wd4267      # Disable size_t conversion warning (CGraph size conversions)
        /wd4100      # Disable unreferenced parameter warning (CGraph unused params)
    )
else()
    set(WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Werror                 # Treat warnings as errors
        -Wno-error=pedantic     # Don't treat pedantic warnings as errors (for third-party headers)
        -Wno-error=format-nonliteral  # Don't treat format-nonliteral as error (CGraph uses vprintf)
        -Wno-error=gnu-zero-variadic-macro-arguments  # Don't treat GNU extensions as errors (CGraph macros)
        -Wno-unknown-warning-option  # Ignore unknown warning options (for GCC-specific warnings on Clang)
        -Wshadow                # Warn about variable shadowing
        -Wnon-virtual-dtor      # Warn about non-virtual destructors
        -Wold-style-cast        # Warn about C-style casts
        -Wcast-align            # Warn about pointer casts with increased alignment
        -Wunused                # Warn about unused variables
        -Woverloaded-virtual    # Warn about overloaded virtual functions
        -Wconversion            # Warn about implicit conversions
        -Wsign-conversion       # Warn about sign conversions
        -Wdouble-promotion      # Warn about float to double promotion
        -Wformat=2              # Extra format string checks
    )
endif()

# Apply warnings to library and executable
target_compile_options(status_rollup_lib PRIVATE ${WARNING_FLAGS})
target_compile_options(rollup PRIVATE ${WARNING_FLAGS})

# ==============================================================================
# Python Bindings
# ==============================================================================
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)

if(BUILD_PYTHON_BINDINGS)
    # Find Python and pybind11
    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

    # Fetch pybind11
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.12.0
    )
    FetchContent_MakeAvailable(pybind11)

    # Create Python module
    pybind11_add_module(_status_rollup python/status_rollup_py.cpp)

    # Link to our library
    target_link_libraries(_status_rollup
        PRIVATE
            status_rollup_lib
    )

    # Set module output location
    set_target_properties(_status_rollup PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/status_rollup"
    )

    # Install the module
    install(TARGETS _status_rollup LIBRARY DESTINATION status_rollup)
endif()

# ==============================================================================
# Testing
# ==============================================================================
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    enable_testing()

    # Fetch GoogleTest
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Unit tests for rollup rules
    add_executable(test_rollup_rules tests/test_rollup_rules.cpp)
    target_link_libraries(test_rollup_rules
        PRIVATE
            status_rollup_lib
            GTest::gtest_main
    )
    target_include_directories(test_rollup_rules
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_compile_options(test_rollup_rules PRIVATE ${WARNING_FLAGS})

    # Integration tests
    add_executable(test_integration tests/test_integration.cpp)
    target_link_libraries(test_integration
        PRIVATE
            status_rollup_lib
            GTest::gtest_main
    )
    target_compile_options(test_integration PRIVATE ${WARNING_FLAGS})

    # Register tests with CTest
    include(GoogleTest)
    gtest_discover_tests(test_rollup_rules)
    gtest_discover_tests(test_integration)

    # Add shell-based integration test
    add_test(
        NAME test_interactive
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_interactive.sh
                $<TARGET_FILE:rollup>
                ${CMAKE_CURRENT_SOURCE_DIR}/examples/complex_status_config.json
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Python tests (if Python bindings are enabled)
    if(BUILD_PYTHON_BINDINGS)
        # Test Python package structure
        add_test(
            NAME test_python_structure
            COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_python_structure.py
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        # Check if pytest is available
        execute_process(
            COMMAND ${Python_EXECUTABLE} -m pytest --version
            RESULT_VARIABLE PYTEST_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(PYTEST_RESULT EQUAL 0)
            # Add pytest tests if available
            add_test(
                NAME pytest
                COMMAND ${Python_EXECUTABLE} -m pytest tests/ -v
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
            # Make pytest depend on the Python module being built
            set_tests_properties(pytest PROPERTIES
                DEPENDS _status_rollup
            )
        else()
            message(STATUS "pytest not found - skipping Python functional tests. Install with: pip install pytest")
        endif()
    endif()
endif()
