#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <cmath>
#include <functional>
#include <exception>
#include <algorithm>

namespace TestFramework {

class TestFailureException : public std::exception {
public:
    TestFailureException(const std::string& file, int line, const std::string& message)
        : file_(file), line_(line), message_(message) {
        std::ostringstream oss;
        oss << file_ << ":" << line_ << ": Failure: " << message_;
        fullMsg_ = oss.str();
    }

    const char* what() const noexcept override {
        return fullMsg_.c_str();
    }

    const std::string& getFile() const { return file_; }
    int getLine() const { return line_; }
    const std::string& getMessage() const { return message_; }

private:
    std::string file_;
    int line_;
    std::string message_;
    std::string fullMsg_;
};

struct TestCaseInfo {
    std::string suite;
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry registry;
        return registry;
    }

    void registerTest(const std::string& suite, const std::string& name, std::function<void()> func) {
        tests_.push_back({suite, name, std::move(func)});
    }

    int runAll(const std::string& filter = "") {
        std::cout << "\n\033[1;36m=================================================================\033[0m\n";
        std::cout << "\033[1;36m                    PARKING LOT SIMULATION TEST SUITE            \033[0m\n";
        std::cout << "\033[1;36m=================================================================\033[0m\n";

        int passed = 0;
        int failed = 0;
        auto startTime = std::chrono::high_resolution_clock::now();

        std::string currentSuite = "";

        for (const auto& test : tests_) {
            if (!filter.empty() && 
                test.suite.find(filter) == std::string::npos && 
                test.name.find(filter) == std::string::npos) {
                continue;
            }

            if (test.suite != currentSuite) {
                currentSuite = test.suite;
                std::cout << "\n\033[1;33m[" << currentSuite << "]\033[0m\n";
            }

            std::cout << "  \033[36m[RUN ]\033[0m " << test.name << "..." << std::flush;
            auto t0 = std::chrono::high_resolution_clock::now();

            try {
                test.func();
                auto t1 = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                std::cout << "\r  \033[1;32m[PASS]\033[0m " << test.name 
                          << " \033[90m(" << ms << " ms)\033[0m\n";
                passed++;
            } catch (const TestFailureException& ex) {
                auto t1 = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                std::cout << "\r  \033[1;31m[FAIL]\033[0m " << test.name 
                          << " \033[90m(" << ms << " ms)\033[0m\n";
                std::cout << "         \033[31m" << ex.what() << "\033[0m\n";
                failed++;
            } catch (const std::exception& ex) {
                auto t1 = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                std::cout << "\r  \033[1;31m[FAIL]\033[0m " << test.name 
                          << " \033[90m(" << ms << " ms)\033[0m\n";
                std::cout << "         \033[31mUnhandled exception: " << ex.what() << "\033[0m\n";
                failed++;
            } catch (...) {
                auto t1 = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
                std::cout << "\r  \033[1;31m[FAIL]\033[0m " << test.name 
                          << " \033[90m(" << ms << " ms)\033[0m\n";
                std::cout << "         \033[31mUnhandled non-standard exception!\033[0m\n";
                failed++;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        double totalMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        std::cout << "\n\033[1;36m=================================================================\033[0m\n";
        std::cout << "Summary: " << passed << " passed, " << failed << " failed, "
                  << (passed + failed) << " total tests in " << totalMs << " ms\n";
        if (failed == 0) {
            std::cout << "\033[1;32mSTATUS: ALL TESTS PASSED!\033[0m\n";
        } else {
            std::cout << "\033[1;31mSTATUS: SOME TESTS FAILED!\033[0m\n";
        }
        std::cout << "\033[1;36m=================================================================\033[0m\n\n";

        return failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCaseInfo> tests_;
};

struct TestRegistrar {
    TestRegistrar(const std::string& suite, const std::string& name, std::function<void()> func) {
        TestRegistry::instance().registerTest(suite, name, std::move(func));
    }
};

} // namespace TestFramework

#define TEST_CASE(suite, name) \
    static void test_##suite##_##name(); \
    static ::TestFramework::TestRegistrar registrar_##suite##_##name(#suite, #name, test_##suite##_##name); \
    static void test_##suite##_##name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream _oss; \
            _oss << "Expected condition to be true: " #cond; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::ostringstream _oss; \
            _oss << "Expected condition to be false: " #cond; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define ASSERT_EQ(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 == _v2)) { \
            std::ostringstream _oss; \
            _oss << "Expected equality: " #val1 " == " #val2 " (Actual: " << _v1 << " vs " << _v2 << ")"; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define ASSERT_NE(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (_v1 == _v2) { \
            std::ostringstream _oss; \
            _oss << "Expected inequality: " #val1 " != " #val2 " (Both: " << _v1 << ")"; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define ASSERT_NEAR(val1, val2, eps) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        auto _e = (eps); \
        if (std::abs(_v1 - _v2) > _e) { \
            std::ostringstream _oss; \
            _oss << "Expected near equality: |" #val1 " - " #val2 "| <= " #eps \
                 << " (| " << _v1 << " - " << _v2 << " | = " << std::abs(_v1 - _v2) << " > " << _e << ")"; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define ASSERT_LT(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 < _v2)) { \
            std::ostringstream _oss; \
            _oss << "Expected less than: " #val1 " < " #val2 " (" << _v1 << " >= " << _v2 << ")"; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define ASSERT_LE(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 <= _v2)) { \
            std::ostringstream _oss; \
            _oss << "Expected less than or equal: " #val1 " <= " #val2 " (" << _v1 << " > " << _v2 << ")"; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define ASSERT_GT(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 > _v2)) { \
            std::ostringstream _oss; \
            _oss << "Expected greater than: " #val1 " > " #val2 " (" << _v1 << " <= " << _v2 << ")"; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define ASSERT_GE(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 >= _v2)) { \
            std::ostringstream _oss; \
            _oss << "Expected greater than or equal: " #val1 " >= " #val2 " (" << _v1 << " < " << _v2 << ")"; \
            throw ::TestFramework::TestFailureException(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#endif // TEST_HARNESS_H
