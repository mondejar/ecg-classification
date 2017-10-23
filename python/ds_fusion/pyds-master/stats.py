"""
Measures the computation time and approximation error of different functions and parameters.

Requires NumPy.
"""

import inspect
import sys
import time
import random
from pyds import MassFunction
import numpy


iterations = 100


def stats(results):
    array = numpy.empty((iterations, 1))
    for i, t in enumerate(results):
        array[i] = t
    return array.mean(), array.std()

def measure_time(f, *args, **kwargs):
    def f_measured(i):
        t = time.clock()
        f(*args, **kwargs)
        return time.clock() - t
    return stats(map(f_measured, range(iterations)))

def measure_error(actual, f, *args, **kwargs):
    return stats(map(lambda _: actual.norm(f(*args, **kwargs)), range(iterations)))

def random_likelihoods(singleton_count):
    return [(i, random.random()) for i in range(singleton_count)]


def time_bel_h():
    return measure_time(MassFunction.gbt(random_likelihoods(12)).bel, hypothesis=frozenset(range(10)))

def time_bel():
    return measure_time(MassFunction({(s,):1.0 for s in range(12)}).normalize().bel)

def time_from_bel():
    bel = MassFunction({(s,):1.0 for s in range(10)}).normalize().bel()
    return measure_time(MassFunction.from_bel, bel)

def time_pl_h():
    return measure_time(MassFunction.gbt(random_likelihoods(12)).pl, frozenset(range(10)))

def time_pl():
    return measure_time(MassFunction({(s,):1.0 for s in range(12)}).normalize().pl)

def time_from_pl():
    pl = MassFunction({(s,):1.0 for s in range(10)}).normalize().pl()
    return measure_time(MassFunction.from_pl, pl)

def time_q_h():
    return measure_time(MassFunction.gbt(random_likelihoods(12)).q, frozenset(range(10)))

def time_q():
    return measure_time(MassFunction({(s,):1.0 for s in range(12)}).normalize().q)

def time_from_q():
    q = MassFunction({(s,):1.0 for s in range(10)}).normalize().q()
    return measure_time(MassFunction.from_q, q)

def time_gbt():
    return measure_time(MassFunction.gbt, random_likelihoods(12))

def time_gbt_100():
    return measure_time(MassFunction.gbt, random_likelihoods(12), sample_count=100)

def time_gbt_1000():
    return measure_time(MassFunction.gbt, random_likelihoods(12), sample_count=1000)

def time_combine_conjunctive():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_time(m1.combine_conjunctive, m2)

def time_combine_conjunctive_1000():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_time(m1.combine_conjunctive, m2, sample_count=1000, importance_sampling=False)

def time_combine_conjunctive_1000_imp():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_time(m1.combine_conjunctive, m2, sample_count=1000, importance_sampling=True)

def time_combine_disjunctive():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_time(m1.combine_disjunctive, m2)

def time_combine_disjunctive_1000():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_time(m1.combine_disjunctive, m2, sample_count=1000)

def time_combine_gbt():
    return measure_time(MassFunction.gbt(random_likelihoods(6)).combine_gbt, random_likelihoods(6))

def time_combine_gbt_1000():
    return measure_time(MassFunction.gbt(random_likelihoods(6)).combine_gbt, random_likelihoods(6), sample_count=1000)

def time_combine_gbt_1000_imp():
    return measure_time(MassFunction.gbt(random_likelihoods(6)).combine_gbt, random_likelihoods(6), sample_count=1000, importance_sampling=True)

def time_pignistic():
    return measure_time(MassFunction.gbt(random_likelihoods(12)).pignistic)

def time_markov():
    m = MassFunction.gbt(random_likelihoods(4))
    return measure_time(MassFunction.gbt(random_likelihoods(4)).markov, lambda s: m)

def time_markov_1000():
    samples = MassFunction.gbt(random_likelihoods(4)).sample(1000)
    return measure_time(MassFunction.gbt(random_likelihoods(4)).markov, lambda s, n: samples[:n], sample_count=1000)


def error_gbt_100():
    lh = random_likelihoods(10)
    return measure_error(MassFunction.gbt(lh), MassFunction.gbt, lh, sample_count=100)

def error_gbt_1000():
    lh = random_likelihoods(10)
    return measure_error(MassFunction.gbt(lh), MassFunction.gbt, lh, sample_count=1000)

def error_combine_conjunctive_100_dir():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_error(m1 & m2, m1.combine_conjunctive, m2, sample_count=100, importance_sampling=False)

def error_combine_conjunctive_100_imp():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_error(m1 & m2, m1.combine_conjunctive, m2, sample_count=100, importance_sampling=True)

def error_combine_conjunctive_1000_dir():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_error(m1 & m2, m1.combine_conjunctive, m2, sample_count=1000, importance_sampling=False)

def error_combine_conjunctive_1000_imp():
    m1 = MassFunction.gbt(random_likelihoods(6))
    m2 = MassFunction.gbt(random_likelihoods(6))
    return measure_error(m1 & m2, m1.combine_conjunctive, m2, sample_count=1000, importance_sampling=True)

def error_combine_gbt_1000_dir():
    m = MassFunction.gbt(random_likelihoods(6))
    lh = random_likelihoods(10)
    return measure_error(m.combine_gbt(lh), m.combine_gbt, lh, sample_count=1000, importance_sampling=False)

def error_combine_gbt_1000_imp():
    m = MassFunction.gbt(random_likelihoods(6))
    lh = random_likelihoods(10)
    return measure_error(m.combine_gbt(lh), m.combine_gbt, lh, sample_count=1000, importance_sampling=True)


def run_measures(prefix):
    mod = sys.modules[__name__]
    filt = lambda x: inspect.isfunction(x) and inspect.getmodule(x) == mod and x.__name__.startswith(prefix + '_')
    print('%-32s%-6s (%4s)' % ('function', 'mean', 'stddev'))
    print('-' * 50)
    for f in sorted(filter(filt, globals().copy().values()), key=str):
        random.seed(0)
        print('%-32s%.4f (+-%.4f)' % ((f.__name__[len(prefix) + 1:],) + f()))


if __name__ == '__main__':
    print('computation time (seconds):')
    run_measures('time')
    print('\n')
    print('approximation error (Euclidean distance):')
    run_measures('error')
