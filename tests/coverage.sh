#!/bin/bash
#
# Phoenix Engine Code Coverage Script
# 
# This script generates code coverage reports for Phoenix Engine tests.
# Requires: gcov, lcov, genhtml
#
# Usage:
#   ./coverage.sh              # Generate coverage report
#   ./coverage.sh clean        # Clean coverage data
#   ./coverage.sh help         # Show this help message
#
# Output:
#   coverage_report/           # HTML coverage report
#   coverage.info              # Raw coverage data
#

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
COVERAGE_DIR="${SCRIPT_DIR}/coverage_report"
COVERAGE_INFO="${BUILD_DIR}/coverage.info"
PROJECT_NAME="Phoenix Engine"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# Helper Functions
# ============================================================================

print_header() {
    echo -e "${BLUE}"
    echo "========================================"
    echo "  ${PROJECT_NAME} Coverage Report"
    echo "========================================"
    echo -e "${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "  $1"
}

check_dependencies() {
    local missing=()
    
    for cmd in gcov lcov genhtml cmake; do
        if ! command -v "$cmd" &> /dev/null; then
            missing+=("$cmd")
        fi
    done
    
    if [ ${#missing[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing[*]}"
        echo ""
        echo "Install with:"
        echo "  Ubuntu/Debian: sudo apt-get install gcovr lcov cmake"
        echo "  macOS:         brew install lcov cmake"
        echo "  Fedora:        sudo dnf install lcov cmake"
        exit 1
    fi
    
    print_success "All dependencies available"
}

clean_coverage() {
    print_info "Cleaning coverage data..."
    
    rm -rf "${COVERAGE_DIR}"
    rm -f "${COVERAGE_INFO}"
    rm -f "${BUILD_DIR}"/*.gcda
    rm -f "${BUILD_DIR}"/*.gcno
    
    print_success "Coverage data cleaned"
}

build_with_coverage() {
    print_info "Configuring build with coverage enabled..."
    
    cmake -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_COVERAGE=ON \
        -S "${SCRIPT_DIR}"
    
    print_success "CMake configuration complete"
    
    print_info "Building project..."
    cmake --build "${BUILD_DIR}" --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    print_success "Build complete"
}

run_tests() {
    print_info "Running tests..."
    
    cd "${BUILD_DIR}"
    ctest --output-on-failure --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    cd "${SCRIPT_DIR}"
    print_success "All tests passed"
}

generate_coverage() {
    print_info "Capturing coverage data..."
    
    # Reset coverage counters
    lcov --directory "${BUILD_DIR}" --zerocounters
    
    # Run tests to generate coverage data
    run_tests
    
    # Capture coverage data
    lcov --directory "${BUILD_DIR}" --capture --output-file "${COVERAGE_INFO}"
    
    print_success "Coverage data captured"
    
    # Remove unwanted coverage data
    print_info "Filtering coverage data..."
    lcov --remove "${COVERAGE_INFO}" \
        '/usr/*' \
        '*/tests/*' \
        '*/test_*' \
        '*/gtest/*' \
        --output-file "${COVERAGE_INFO}"
    
    print_success "Coverage data filtered"
}

generate_html_report() {
    print_info "Generating HTML report..."
    
    genhtml "${COVERAGE_INFO}" \
        --output-directory "${COVERAGE_DIR}" \
        --title "${PROJECT_NAME} Coverage Report" \
        --legend \
        --num-spaces 4 \
        --show-details \
        --highlight \
        --ignore-errors source
    
    print_success "HTML report generated: ${COVERAGE_DIR}/index.html"
}

print_summary() {
    echo ""
    print_header
    
    if [ -f "${COVERAGE_INFO}" ]; then
        print_info "Coverage Summary:"
        lcov --summary "${COVERAGE_INFO}"
    fi
    
    echo ""
    print_info "To view the coverage report:"
    echo "  open ${COVERAGE_DIR}/index.html"
    echo ""
    print_info "Or start a local server:"
    echo "  cd ${COVERAGE_DIR} && python3 -m http.server 8080"
    echo "  Then open: http://localhost:8080"
    echo ""
}

show_help() {
    echo "Phoenix Engine Code Coverage Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  (none)     Generate full coverage report (build, test, report)"
    echo "  clean      Remove all coverage data"
    echo "  build      Build with coverage enabled (no tests)"
    echo "  test       Run tests and generate coverage (assumes built)"
    echo "  report     Generate HTML report from existing coverage data"
    echo "  help       Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0              # Full coverage workflow"
    echo "  $0 clean        # Clean coverage files"
    echo "  $0 test         # Run tests and capture coverage"
    echo ""
    echo "Output:"
    echo "  coverage_report/    HTML coverage report"
    echo "  coverage.info       Raw coverage data (LCOV format)"
    echo ""
}

# ============================================================================
# Main Script
# ============================================================================

main() {
    print_header
    
    case "${1:-}" in
        clean)
            check_dependencies
            clean_coverage
            ;;
        build)
            check_dependencies
            build_with_coverage
            ;;
        test)
            check_dependencies
            if [ ! -d "${BUILD_DIR}" ]; then
                print_warning "Build directory not found. Building first..."
                build_with_coverage
            fi
            generate_coverage
            generate_html_report
            print_summary
            ;;
        report)
            check_dependencies
            if [ ! -f "${COVERAGE_INFO}" ]; then
                print_error "Coverage data not found. Run '$0 test' first."
                exit 1
            fi
            generate_html_report
            print_summary
            ;;
        help|--help|-h)
            show_help
            ;;
        "")
            check_dependencies
            build_with_coverage
            generate_coverage
            generate_html_report
            print_summary
            ;;
        *)
            print_error "Unknown command: $1"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
