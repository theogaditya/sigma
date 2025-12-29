#ifndef SIGMA_TEST_FRAMEWORK_H
#define SIGMA_TEST_FRAMEWORK_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace sigma {
namespace test {

// ============================================================================
// Test Result Tracking
// ============================================================================

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    double durationMs;
};

class TestRunner {
public:
    static TestRunner& instance() {
        static TestRunner instance;
        return instance;
    }

    void addTest(const std::string& name, std::function<bool()> test) {
        tests_.push_back({name, std::move(test)});
    }

    int run() {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════╗\n";
        std::cout << "║          Sigma Language Test Suite                   ║\n";
        std::cout << "╚══════════════════════════════════════════════════════╝\n\n";

        int passed = 0;
        int failed = 0;

        for (auto& [name, test] : tests_) {
            auto start = std::chrono::high_resolution_clock::now();
            
            bool result = false;
            std::string errorMsg;
            
            try {
                result = test();
            } catch (const std::exception& e) {
                errorMsg = e.what();
            } catch (...) {
                errorMsg = "Unknown exception";
            }

            auto end = std::chrono::high_resolution_clock::now();
            double durationMs = std::chrono::duration<double, std::milli>(end - start).count();

            if (result) {
                passed++;
                std::cout << "  \033[1;32m✓\033[0m " << name 
                         << " \033[90m(" << std::fixed << std::setprecision(2) 
                         << durationMs << "ms)\033[0m\n";
            } else {
                failed++;
                std::cout << "  \033[1;31m✗\033[0m " << name << "\n";
                if (!errorMsg.empty()) {
                    std::cout << "    \033[31mError: " << errorMsg << "\033[0m\n";
                }
            }

            results_.push_back({name, result, errorMsg, durationMs});
        }

        std::cout << "\n";
        std::cout << "────────────────────────────────────────────────────────\n";
        std::cout << "  Total: " << (passed + failed) 
                 << "  \033[32mPassed: " << passed << "\033[0m"
                 << "  \033[31mFailed: " << failed << "\033[0m\n";
        std::cout << "────────────────────────────────────────────────────────\n\n";

        return failed > 0 ? 1 : 0;
    }

    void reset() {
        tests_.clear();
        results_.clear();
    }

    const std::vector<TestResult>& results() const { return results_; }

private:
    TestRunner() = default;
    std::vector<std::pair<std::string, std::function<bool()>>> tests_;
    std::vector<TestResult> results_;
};

// ============================================================================
// Test Registration Macro
// ============================================================================

#define TEST(name) \
    static bool test_##name(); \
    namespace { \
        struct TestRegister_##name { \
            TestRegister_##name() { \
                sigma::test::TestRunner::instance().addTest(#name, test_##name); \
            } \
        }; \
        static TestRegister_##name register_##name; \
    } \
    static bool test_##name()

// ============================================================================
// Assertions
// ============================================================================

#define EXPECT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "    Expected true: " << #expr << "\n"; \
        return false; \
    }

#define EXPECT_FALSE(expr) \
    if (expr) { \
        std::cerr << "    Expected false: " << #expr << "\n"; \
        return false; \
    }

#define EXPECT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "    Expected equal: " << #a << " == " << #b << "\n"; \
        std::cerr << "    Got: " << (a) << " vs " << (b) << "\n"; \
        return false; \
    }

#define EXPECT_NE(a, b) \
    if ((a) == (b)) { \
        std::cerr << "    Expected not equal: " << #a << " != " << #b << "\n"; \
        return false; \
    }

#define EXPECT_LT(a, b) \
    if (!((a) < (b))) { \
        std::cerr << "    Expected " << (a) << " < " << (b) << "\n"; \
        return false; \
    }

#define EXPECT_GT(a, b) \
    if (!((a) > (b))) { \
        std::cerr << "    Expected " << (a) << " > " << (b) << "\n"; \
        return false; \
    }

#define EXPECT_THROW(expr, exception_type) \
    try { \
        expr; \
        std::cerr << "    Expected exception: " << #exception_type << "\n"; \
        return false; \
    } catch (const exception_type&) { \
    } catch (...) { \
        std::cerr << "    Wrong exception type thrown\n"; \
        return false; \
    }

#define EXPECT_NO_THROW(expr) \
    try { \
        expr; \
    } catch (const std::exception& e) { \
        std::cerr << "    Unexpected exception: " << e.what() << "\n"; \
        return false; \
    } catch (...) { \
        std::cerr << "    Unexpected exception thrown\n"; \
        return false; \
    }

} // namespace test
} // namespace sigma

#endif // SIGMA_TEST_FRAMEWORK_H
