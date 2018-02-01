#!/usr/bin/env python

"""
train_SVM.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
15 Dec 2017
"""

import os
import csv
import gc
import cPickle as pickle
import time
from imblearn.over_sampling import SMOTE, ADASYN
from imblearn.combine import SMOTEENN, SMOTETomek
import collections
from sklearn import svm
import numpy as np

cpu_threads = 7

# http://contrib.scikit-learn.org/imbalanced-learn/stable/auto_examples/combine/plot_comparison_combine.html#sphx-glr-auto-examples-combine-plot-comparison-combine-py

# Perform the oversampling method over the descriptor data
def perform_oversampling(oversamp_method, db_path, oversamp_features_name, tr_features, tr_labels):
    start = time.time()
    
    oversamp_features_pickle_name = db_path + oversamp_features_name + '_' + oversamp_method + '.p'
    print(oversamp_features_pickle_name)

    if True:
        print("Oversampling method:\t" + oversamp_method + " ...")
        # 1 SMOTE
        if oversamp_method == 'SMOTE':  
            #kind={'borderline1', 'borderline2', 'svm'}
            svm_model = svm.SVC(C=0.001, kernel='rbf', degree=3, gamma='auto', decision_function_shape='ovo')
            oversamp = SMOTE(ratio='auto', random_state=None, k_neighbors=5, m_neighbors=10, out_step=0.5, kind='svm', svm_estimator=svm_model, n_jobs=1)

            # PROBAR SMOTE CON OTRO KIND

        elif oversamp_method == 'SMOTE_regular_min':
            oversamp = SMOTE(ratio='minority', random_state=None, k_neighbors=5, m_neighbors=10, out_step=0.5, kind='regular', svm_estimator=None, n_jobs=1)

        elif oversamp_method == 'SMOTE_regular':
            oversamp = SMOTE(ratio='auto', random_state=None, k_neighbors=5, m_neighbors=10, out_step=0.5, kind='regular', svm_estimator=None, n_jobs=1)
  
        elif oversamp_method == 'SMOTE_border':
            oversamp = SMOTE(ratio='auto', random_state=None, k_neighbors=5, m_neighbors=10, out_step=0.5, kind='borderline1', svm_estimator=None, n_jobs=1)
                 
        # 2 SMOTEENN
        elif oversamp_method == 'SMOTEENN':    
            oversamp = SMOTEENN()

        # 3 SMOTE TOMEK
        # NOTE: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.65.3904&rep=rep1&type=pdf
        elif oversamp_method == 'SMOTETomek':
            oversamp = SMOTETomek()

        # 4 ADASYN
        elif oversamp_method == 'ADASYN':
            oversamp = ADASYN(ratio='auto', random_state=None, k=None, n_neighbors=5, n_jobs=cpu_threads)
 
        tr_features_balanced, tr_labels_balanced  = oversamp.fit_sample(tr_features, tr_labels)
        # TODO Write data oversampled!
        print("Writing oversampled data at: " + oversamp_features_pickle_name + " ...")
        np.savetxt('mit_db/' + oversamp_features_name + '_DS1_labels.csv', tr_labels_balanced.astype(int), '%.0f') 
        f = open(oversamp_features_pickle_name, 'wb')
        pickle.dump(tr_features_balanced, f, 2)
        f.close

    end = time.time()

    count = collections.Counter(tr_labels_balanced)
    print("Oversampling balance")
    print(count)
    print("Time required: " + str(format(end - start, '.2f')) + " sec" )

    return tr_features_balanced, tr_labels_balanced 
