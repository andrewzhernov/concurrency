#!/usr/bin/env python3

def output_test(test_num, test_data):
    with open("{test_num:02d}.in".format(test_num=test_num), "w") as f:
        print(test_data, file=f)

def generate_tests():
    for foots, steps in [
        (2,  10),
        (1,  10000),
        (2,  10000),
        (3,  10000),
        (4,  10000),
        (5,  10000),
        (10,  5000),
        (15,  5000),
        (20,  2000),
        (25,  2000),
    ]:
        yield "{foots} {steps}".format(**locals())

def main():
    for test_num, test_data in enumerate(generate_tests()):
        output_test(test_num, test_data)

if __name__ == "__main__":
    main()

