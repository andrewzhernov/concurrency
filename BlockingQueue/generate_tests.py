#!/usr/bin/env python3

import sys
import argparse

def make_range(s):
    r = (start, end, step) = [int(x) for x in s.split(',')]
    return r

def make_log_range(s):
    r = (base, start, end, step) = [int(x) for x in s.split(',')]
    return r

def log_range(base, start, end, step):
    for p in range(start, end, step):
        yield base ** p
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--capacity_log_range', metavar='base,start,end,step', type=make_log_range, required=True)
    parser.add_argument('-r', '--readers_range', metavar='start,end,step', type=make_range, required=True)
    parser.add_argument('-w', '--writers_range', metavar='start,end,step', type=make_range, required=True)
    parser.add_argument('-i', '--items_log_range', metavar='base,start,end,step', type=make_log_range, required=True)
    args = parser.parse_args()
    test_num = 1
    
    for capacity in log_range(*args.capacity_log_range):
        for readers in range(*args.readers_range):
            for writers in range(*args.writers_range):
                for items in log_range(*args.items_log_range):
                    with open("{test_num:02d}.in".format(**locals()), "w") as f:
                        print("{capacity} {readers} {writers} {items}".format(**locals()), file=f)
                    test_num += 1
    