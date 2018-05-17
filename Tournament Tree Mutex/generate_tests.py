#!/usr/bin/env python3

def log_range(base, start, end, step):
    for p in range(start, end, step):
        yield base ** p


def output_test(test_num, test_data):
    with open("{test_num:02d}.in".format(test_num=test_num), "w") as f:
        print(test_data, file=f)


def generate_tests():
    for threads, iters, shareds in [
        (1,  500000, 1),
        (2,  500000, 0),
        (2,  500000, 1),
        (2,  50000, 10),
        (3,  330000, 0),
        (3,  330000, 1),
        (3,  33000, 10),
        (4,  250000, 0),
        (4,  250000, 1),
        (4,  25000, 10),
        (5,  200000, 0),
        (5,  200000, 1),
        (5,  20000, 10),
        (10, 100000, 0),
        (10, 50000, 1),
        (10, 5000, 10),
    ]:
        yield "{threads} {iters} {shareds}".format(**locals())


def main():
    for test_num, test_data in enumerate(generate_tests()):
        output_test(test_num, test_data)


main()
