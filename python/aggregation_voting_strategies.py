#!/usr/bin/env python

"""
aggregation_voting_strategies.py

Description:
Contains the methods for combining the decisions given by 
multiclass SVM models: OvO and OvR 

VARPA, University of Coruna
Mondejar Guerra, Victor M.
13 Nov 2017
"""

import numpy as np
from evaluation_AAMI import *
from sklearn import svm
import time


# NOTE: use direct decision values or transform to range (0,1) with
# something like: 1/exp(-decision) ?

# Each classifier gives only one vote for its higher class.
# Then the majority vote rule is applied to select the final prediction
def ovo_voting(decision_ovo, n_classes):
    predictions = np.zeros(len(decision_ovo))
    class_pos, class_neg = ovo_class_combinations(n_classes)

    counter = np.zeros([len(decision_ovo), n_classes])

    for p in range(len(decision_ovo)):
        for i in range(len(decision_ovo[p])):
            if decision_ovo[p,i] > 0:
                counter[p, class_pos[i]] += 1
            else:
                counter[p, class_neg[i]] += 1

        predictions[p] = np.argmax(counter[p])

    return predictions, counter


def ovo_voting_exp(decision_ovo, n_classes):
    predictions = np.zeros(len(decision_ovo))
    class_pos, class_neg = ovo_class_combinations(n_classes)

    counter = np.zeros([len(decision_ovo), n_classes])

    for p in range(len(decision_ovo)):
        for i in range(len(decision_ovo[p])):
            counter[p, class_pos[i]] += 1 / (1 + np.exp(-decision_ovo[p,i]) )
            counter[p, class_neg[i]] += 1 / (1 + np.exp( decision_ovo[p,i]) )

        predictions[p] = np.argmax(counter[p])

    return predictions, counter




####################################################
# Testing new functions...


# http://sci2s.ugr.es/ovo-ova

def ovo_class_combinations(n_classes):
    class_pos = []
    class_neg = []
    for c1 in range(n_classes-1):
        for c2 in range(c1+1,n_classes):
            class_pos.append(c1)
            class_neg.append(c2)

    return class_pos, class_neg

# Each classifier adds it value for both classes (+/-)
# Then the class with largest number of votes is the prediction
def ovo_voting_both(decision_ovo, n_classes):
    
    predictions = np.zeros(len(decision_ovo))
    class_pos, class_neg = ovo_class_combinations(n_classes)

    counter = np.zeros([len(decision_ovo), n_classes])

    for p in range(len(decision_ovo)):
        for i in range(len(decision_ovo[p])):
            counter[p, class_pos[i]] += decision_ovo[p,i] 
            counter[p, class_neg[i]] -= decision_ovo[p,i] 

        predictions[p] = np.argmax(counter[p])

    return predictions, counter


def ovo_voting_both2(decision_ovo, n_classes):
    
    predictions = np.zeros(len(decision_ovo))
    class_pos, class_neg = ovo_class_combinations(n_classes)

    counter = np.zeros([len(decision_ovo), n_classes])

    for p in range(len(decision_ovo)):
        for i in range(len(decision_ovo[p])):
            if decision_ovo[p,i] != 0.0:
                counter[p, class_pos[i]] += (decision_ovo[p,i]  - 0.5)
                counter[p, class_neg[i]] += (0.5 - decision_ovo[p,i])

        predictions[p] = np.argmax(counter[p])

    return predictions, counter

# https://papers.nips.cc/paper/1773-large-margin-dags-for-multiclass-classification.pdf
#def ovo_DDAG(): # Decision Directed Acyclic Graph 


# https://link.springer.com/content/pdf/10.1007%2Fs00500-006-0093-3.pdf
def ovo_fuzzy(decision_ovo, n_classes):
    
    predictions = np.zeros(len(decision_ovo))
    class_pos, class_neg = ovo_class_combinations(n_classes)

    counter = np.zeros([len(decision_ovo), n_classes])

    for p in range(len(decision_ovo)):
        for i in range(len(decision_ovo[p])):


            # TODO... continuar esta idea..abs

            counter[p, class_pos[i]] = counter[p, class_pos[i]] + (decision_ovo[p,i] if decision_ovo[p,i] < 1 else 1 )
            counter[p, class_neg[i]] = counter[p, class_neg[i]] (-decision_ovo[p,i] if decision_ovo[p,i] > -1 else -1 )

        predictions[p] = np.argmax(counter[p])

    return predictions, counter
