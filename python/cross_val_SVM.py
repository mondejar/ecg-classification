#!/usr/bin/env python

"""
cross_val_SVM.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
2 Nov 2017
"""

from train_SVM import svm


def main(winL=90, winR=90, do_preprocess=True, use_weight_class=True, 
    maxRR=True, use_RR=True, norm_RR=True, compute_morph={''}):

    print("Runing cross_val_SVM.py!")
