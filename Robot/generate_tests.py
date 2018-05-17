#!/usr/bin/env python3

def log_range(base, start, end, step):
    for p in range(start, end, step):
        yield base ** p


def output_test(test_num, test_data):
    with open("{test_num:02d}.in".format(test_num=test_num), "w") as f:
        print(test_data, file=f)


def generate_tests():
    yield ""
    yield ""
    yield ""
    yield ""
    yield ""
    yield ""
    yield ""
    yield ""
    yield ""
    yield ""


def main():
    for test_num, test_data in enumerate(generate_tests()):
        output_test(test_num, test_data)


main()
