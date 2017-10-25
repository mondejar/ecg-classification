#!/usr/bin/env python

"""
train_SVM.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
23 Oct 2017
"""

from load_MITBIH import *

def main(args):
    print("Train SVM !")

    # Define configuration
    winL = 90
    winR = 90
    do_preprocess = True
    maxRR = True
    use_RR = True
    norm_RR = True
    compute_morph = {'wavelets', 'HOS', 'myMorph'}
    
    # Load data 
    [features, labels] = load_mit_db('DS1', winL, winR, do_preprocess, maxRR, use_RR, norm_RR, compute_morph)

    # Train

if __name__ == '__main__':
    import sys
    main(sys.argv[1:])