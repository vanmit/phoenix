# Sanitizers Configuration for Phoenix Engine
# Enable AddressSanitizer (ASAN), UndefinedBehaviorSanitizer (UBSAN), and more

# Usage:
#   cmake -DENABLE_SANITIZERS=ON -DSANITIZER_TYPES="address,undefined" ..
#   cmake --build .
#   ./phoenix_tests

option(ENABLE_SANITIZERS "Enable sanitizer instrumentation" OFF)
option(SANITIZER_TYPES "Comma-separated list of sanitizers: address,undefined,thread,memory,leak" "address,undefined")

if(ENABLE_SANITIZERS)
    message(STATUS "🛡️  Sanitizers enabled: ${SANITIZER_TYPES}")
    
    # Parse sanitizer types
    string(REPLACE "," ";" SANITIZER_LIST "${SANITIZER_TYPES}")
    
    set(SANITIZER_FLAGS "")
    set(SANITIZER_LINK_FLAGS "")
    
    foreach(SANITIZER ${SANITIZER_LIST})
        if(SANITIZER STREQUAL "address")
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
            set(SANITIZER_LINK_FLAGS "${SANITIZER_LINK_FLAGS} -fsanitize=address")
            message(STATUS "  ✓ AddressSanitizer (ASAN) enabled")
        elseif(SANITIZER STREQUAL "undefined")
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=undefined")
            set(SANITIZER_LINK_FLAGS "${SANITIZER_LINK_FLAGS} -fsanitize=undefined")
            message(STATUS "  ✓ UndefinedBehaviorSanitizer (UBSAN) enabled")
        elseif(SANITIZER STREQUAL "thread")
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=thread")
            set(SANITIZER_LINK_FLAGS "${SANITIZER_LINK_FLAGS} -fsanitize=thread")
            message(STATUS "  ✓ ThreadSanitizer (TSAN) enabled")
        elseif(SANITIZER STREQUAL "memory")
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=memory -fno-omit-frame-pointer")
            set(SANITIZER_LINK_FLAGS "${SANITIZER_LINK_FLAGS} -fsanitize=memory")
            message(STATUS "  ✓ MemorySanitizer (MSAN) enabled")
        elseif(SANITIZER STREQUAL "leak")
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=leak")
            set(SANITIZER_LINK_FLAGS "${SANITIZER_LINK_FLAGS} -fsanitize=leak")
            message(STATUS "  ✓ LeakSanitizer (LSAN) enabled")
        endif()
    endforeach()
    
    # Apply sanitizer flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_LINK_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_LINK_FLAGS}")
    
    # ASAN options (can be overridden via ASAN_OPTIONS env var)
    set(ENV{ASAN_OPTIONS} "detect_leaks=1:abort_on_error=1:symbolize=1:check_initialization_order=1:strict_init_order=1")
    set(ENV{UBSAN_OPTIONS} "print_stacktrace=1:abort_on_error=1")
    
    message(STATUS "  📋 ASAN_OPTIONS: detect_leaks=1:abort_on_error=1:symbolize=1")
    message(STATUS "  📋 UBSAN_OPTIONS: print_stacktrace=1:abort_on_error=1")
    
    # Disable for Release builds (performance impact)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(WARNING "⚠️  Sanitizers are typically used with Debug or RelWithDebInfo builds")
    endif()
endif()

# ============================================================================
# Valgrind Support (alternative to sanitizers)
# ============================================================================

find_program(VALGRIND_PROGRAM valgrind)

if(VALGRIND_PROGRAM)
    message(STATUS "✓ Valgrind found: ${VALGRIND_PROGRAM}")
    
    # Create valgrind test target
    add_custom_target(valgrind_tests
        COMMAND ${VALGRIND_PROGRAM} 
            --leak-check=full 
            --show-leak-kinds=all 
            --track-origins=yes 
            --verbose 
            --error-exitcode=1
            $<TARGET_FILE:phoenix_tests>
        DEPENDS phoenix_tests
        COMMENT "Running tests with Valgrind memory checker"
    )
else()
    message(STATUS "⚠️  Valgrind not found - install for additional memory checking")
endif()

# ============================================================================
# Cppcheck Static Analysis
# ============================================================================

find_program(CPPCHECK_PROGRAM cppcheck)

if(CPPCHECK_PROGRAM)
    message(STATUS "✓ Cppcheck found: ${CPPCHECK_PROGRAM}")
    
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_PROGRAM}
            --enable=all
            --inconclusive
            --std=c++20
            --platform=unix64
            -I${CMAKE_SOURCE_DIR}/include
            --suppress=missingIncludeSystem
            --error-exitcode=1
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/tests
        COMMENT "Running Cppcheck static analysis"
    )
else()
    message(STATUS "⚠️  Cppcheck not found - install for static analysis")
endif()

# ============================================================================
# Usage Examples
# ============================================================================
#
# # Build with ASAN + UBSAN
# mkdir build && cd build
# cmake -DENABLE_SANITIZERS=ON -DSANITIZER_TYPES="address,undefined" -DCMAKE_BUILD_TYPE=Debug ..
# cmake --build .
#
# # Run tests with sanitizers
# ./phoenix_tests
#
# # Run with custom ASAN options
# ASAN_OPTIONS=detect_stack_use_after_return=1 ./phoenix_tests
#
# # Run with Valgrind
# make valgrind_tests
#
# # Run static analysis
# make cppcheck
#
# # Build fuzz targets (requires cargo-fuzz)
# cd security-audit/fuzz
# cargo fuzz build
# cargo fuzz run crypto_encrypt_fuzzer
#
