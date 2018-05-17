#pragma once

#include <exception>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>

//////////////////////////////////////////////////////////////////////

class TestException : public std::exception {
public:
    TestException() = default;

    TestException(TestException&& e) noexcept {
        (*this) = std::move(e);
    }

    TestException& operator=(TestException&& e) noexcept {
        descr_ = std::move(e.descr_);
        (std::exception&)(*this) = std::move(e);
        return *this;
    }

    explicit TestException(const std::string& descr)
        : descr_(descr) {
    }

    const char* what() const noexcept override {
        return descr_.c_str();
    }

private:
    std::string descr_;
};

//////////////////////////////////////////////////////////////////////

#if defined(cthrow) || defined(cabort) || defined(cdebug)
#   error "already defined"
#else
#   define cdebug(what) do { break; \
    std::ostringstream s; \
    s << std::this_thread::get_id() << ":" << __LINE__ << ": " << what << "\n"; \
    std::cerr << s.str(); \
} while (false)
#   define cthrow(what) do { \
    std::ostringstream s; \
    s << "exception at [Thread " << std::this_thread::get_id() << "] " << __PRETTY_FUNCTION__ << ":" << __LINE__ << ": " << what; \
    throw TestException(s.str()); \
} while (false)
#   define cabort(what) do { \
    std::ostringstream s; \
    s << "abort at [Thread " << std::this_thread::get_id() << "] " << __PRETTY_FUNCTION__ << ":" << __LINE__ << ": " << what << std::endl; \
    std::cerr << s.str(); \
    abort(); \
} while (false)
#endif

#if defined(test_assert)
#   error "already defined"
#else
#   define test_assert(cond, comment) do { \
    const bool succeeded = (cond); \
    if (!succeeded) { \
        cabort("assert " << comment << " (" #cond ") failed"); \
    } \
} while (false)
#endif

//////////////////////////////////////////////////////////////////////
