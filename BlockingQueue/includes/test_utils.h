#pragma once

#include <exception>
#include <sstream>
#include <fstream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

namespace QueueTests {

class TestException : public std::exception {
    std::string descr;
public:
    TestException() = default;

    TestException(TestException&& c) noexcept {
        (*this) = std::move(c);
    }

    TestException& operator=(TestException&& c) noexcept {
        descr = std::move(c.descr);
        (std::exception&)(*this) = std::move(c);
        return *this;
    }

    explicit TestException(const std::string& d)
        : descr(d)
    {}

    const char* what() const noexcept override {
        return descr.c_str();
    }
};

#if defined(cthrow) || defined(cabort)
#   error "already defined"
#else
#   define cthrow(what) do { \
    std::ostringstream s; \
    s << "exception at " << __PRETTY_FUNCTION__ << ":" << __LINE__ << ": " << what; \
    throw TestException(s.str()); \
} while (false)
#   define cabort(what) do { \
    std::ostringstream s; \
    s << "abort at " << __PRETTY_FUNCTION__ << ":" << __LINE__ << ": " << what << std::endl; \
    std::cerr << s.str(); \
    abort(); \
} while (false)
#endif

#if defined(test_assert)
#   error "already defined"
#else
#   define test_assert(cond, comment) do { \
    bool res = (cond); \
    if (!res) { \
        std::string what; \
        cabort("assert " << comment << " (" #cond ") failed"); \
    } \
} while (false)
#endif

///////////////////////////////////////////////////////////
// Barrier to force all threads to start at the same time
///////////////////////////////////////////////////////////

class Barrier {
public:
    explicit Barrier(size_t cnt)
        : count(cnt)
    {
    }

    void wait() {
        std::unique_lock<std::mutex> lock{mutex};
        --count;
        if (!count) {
            cond.notify_all();
        } else {
            cond.wait(lock, [this]() { return count == 0; });
        }
    }

private:
    std::mutex mutex;
    std::condition_variable cond;
    size_t count;
};

/////////////////////////
// argument parsing utils
/////////////////////////

template <typename Opt>
void do_parse_opts(std::basic_istream<char>& inp, Opt& opt) {
    if (!inp) {
        cthrow("invalid args");
    }

    inp >> opt;
}

template <typename Opt, typename ... Opts>
void do_parse_opts(std::basic_istream<char>& inp, Opt& opt, Opts& ... opts) {
    do_parse_opts(inp, opt);
    do_parse_opts(inp, opts...);
}

template <typename ... Opts>
void do_read_opts(int argc, char* argv[], const char* usage, Opts& ... opts) {
    try {
        if (argc < 2 || argv[1] == std::string("-")) {
            do_parse_opts(std::cin, opts...);
        } else if (argc >= 2 && argv[1] == std::string("--")) {
            std::stringstream str;
            for (int i = 2; i < argc; ++i) {
                str << argv[i] << " ";
            }
            do_parse_opts(str, opts...);
        } else if (argc >= 2 && argv[1] != std::string("--help")) {
            std::ifstream fin(argv[1]);
            do_parse_opts(fin, opts...);
        } else {
            cthrow("invalid options");
        }
    } catch (const std::exception& e) {
        std::cerr << "usage: " << argv[0] << " ( | - | input.txt | --help | -- " << usage << " )" << std::endl;

        if (argc >= 2 && argv[1] == std::string("--help")) {
            exit(0);
        } else {
            throw;
        }
    }
}

#if defined(read_opts)
#   error "already defined"
#else
#   define read_opts(argc, argv, ...) do_read_opts(argc, argv, #__VA_ARGS__, ##__VA_ARGS__)
#endif

}
