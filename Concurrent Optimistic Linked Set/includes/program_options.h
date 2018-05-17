#pragma once

#include <exception>
#include <sstream>
#include <fstream>
#include <string>
#include <iostream>

//////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////

#if defined(read_opts)
#   error "already defined"
#else
#   define read_opts(argc, argv, ...) do_read_opts(argc, argv, #__VA_ARGS__, ##__VA_ARGS__)
#endif

//////////////////////////////////////////////////////////////////////
