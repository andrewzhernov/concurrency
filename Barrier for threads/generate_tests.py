#!/usr/bin/env python3

def log_range(base, start, end, step):
    for p in range(start, end, step):
        yield base ** p


def output_test(test_num, test_data):
    with open("{test_num:02d}.in".format(test_num=test_num), "w") as f:
        print(test_data, file=f)


def generate_tests():
    for threads, iters in [
        (1,  500000),
        (2,  50000),
        (3,  33000),
        (4,  25000),
        (5,  20000),
        (10, 10000),
        (20, 5000),
        (25, 5000),
    ]:
        yield "{threads} {iters}".format(**locals())


def main():
    for test_num, test_data in enumerate(generate_tests()):
        output_test(test_num, test_data)


main()
