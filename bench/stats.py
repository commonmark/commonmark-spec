#!/usr/bin/env python3

import sys
import statistics

values = [ float(x) for x in sys.stdin.readlines()]

print("mean = %.4f, median = %.4f, stdev = %.4f, variance = %.4f" %
    (statistics.mean(values), statistics.median(values),
      statistics.stdev(values), statistics.variance(values)))

