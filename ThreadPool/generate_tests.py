#!/usr/bin/env python
from __future__ import print_function
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
    parser.add_argument('-w', '--workers_range', nargs='+', metavar='start,end,step', type=make_range, required=True)
    parser.add_argument('-i', '--items_log_range', nargs='+', metavar='base,start,end,step', type=make_log_range, required=True)
    args = parser.parse_args()
    test_num = 1
    
    for workers_range in args.workers_range:
        for items_log_range in args.items_log_range:
            for workers in range(*workers_range):
                for items in log_range(*items_log_range):
                    with open("{test_num:02d}.in".format(**locals()), "w") as f:
                        print("{workers} {items}".format(**locals()), file=f)
                    test_num += 1
