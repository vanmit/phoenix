/**
 * @file test_main.cpp
 * @brief Phoenix Engine Test Entry Point
 * 
 * This file provides the main entry point for running all Phoenix Engine tests.
 * It initializes the test framework and executes all registered test cases.
 */

#include <gtest/gtest.h>
#include <iostream>
#include <cstdlib>

/**
 * @brief Custom test event listener for detailed test output
 */
class PhoenixTestListener : public testing::EmptyTestEventListener {
public:
    void OnTestProgramStart(const testing::UnitTest& unit_test) override {
        std::cout << "========================================\n";
        std::cout << "  Phoenix Engine Test Suite Starting\n";
        std::cout << "========================================\n";
        std::cout << "Total test cases: " << unit_test.test_to_run_count() << "\n\n";
    }

    void OnTestIterationStart(const testing::UnitTest& unit_test, int iteration) override {
        std::cout << "Running test iteration " << (iteration + 1) << "...\n\n";
    }

    void OnTestEnd(const testing::TestInfo& test_info) override {
        if (test_info.result()->Passed()) {
            std::cout << "  ✓ " << test_info.test_case_name() 
                      << "." << test_info.name() << "\n";
        } else {
            std::cout << "  ✗ " << test_info.test_case_name() 
                      << "." << test_info.name() << " [FAILED]\n";
        }
    }

    void OnTestProgramEnd(const testing::UnitTest& unit_test) override {
        std::cout << "\n========================================\n";
        std::cout << "  Test Results Summary\n";
        std::cout << "========================================\n";
        std::cout << "Total tests: " << unit_test.test_to_run_count() << "\n";
        std::cout << "Passed: " << unit_test.successful_test_count() << "\n";
        std::cout << "Failed: " << unit_test.failed_test_count() << "\n";
        std::cout << "Disabled: " << unit_test.disabled_test_count() << "\n";
        std::cout << "Skipped: " << unit_test.skipped_test_count() << "\n";
        std::cout << "Time: " << unit_test.elapsed_time() << " ms\n";
        std::cout << "========================================\n";
    }
};

/**
 * @brief Initialize Phoenix Engine test environment
 */
void InitializePhoenixEngine() {
    // Initialize any Phoenix Engine subsystems required for testing
    // This is a placeholder for actual initialization code
    std::cout << "Initializing Phoenix Engine test environment...\n";
}

/**
 * @brief Cleanup Phoenix Engine test environment
 */
void CleanupPhoenixEngine() {
    // Cleanup any Phoenix Engine subsystems after testing
    std::cout << "Cleaning up Phoenix Engine test environment...\n";
}

/**
 * @brief Main test entry point
 */
int main(int argc, char** argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // Add custom test listener
    testing::UnitTest::GetInstance()->listeners().Append(new PhoenixTestListener);

    // Initialize Phoenix Engine
    InitializePhoenixEngine();

    // Run all tests
    int result = RUN_ALL_TESTS();

    // Cleanup
    CleanupPhoenixEngine();

    return result;
}
