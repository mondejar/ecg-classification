#!/usr/bin/env python

"""
load_MITBIH.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
23 Oct 2017
"""

import os
import csv
import gc
import cPickle as pickle

import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import medfilt
import scipy.stats
import pywt

from features_ECG import *


# Load the data with the configuration and features selected
def load_mit_db(DS, winL, winR, do_preprocess, maxRR, use_RR, norm_RR, compute_morph, db_path):

    # TODO export features,labels .csv? 
    # If exist the same configuration and features selected load the .csv file!

    print("Loading MIT BIH arr (" + DS + ") ...")

    DS1 = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230]
    DS2 = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234]

    mit_pickle_name = db_path + 'python_mit'
    if do_preprocess:
        mit_pickle_name = mit_pickle_name + '_rm_bsline'

    mit_pickle_name = mit_pickle_name + '_wL_' + str(winL) + '_wR_' + str(winR)
    mit_pickle_name = mit_pickle_name + '_' + DS + '.p'

    # If the data with that configuration has been already computed Load pickle
    if os.path.isfile(mit_pickle_name):
        f = open(mit_pickle_name, 'rb')
        # disable garbage collector       
        gc.disable()# this improve the required loading time!
        my_db = pickle.load(f)
        gc.enable()
        f.close()

    else: # Load data and compute de RR features 
        if DS == 'DS1':
            my_db = load_signal(DS1, winL, winR, do_preprocess)
        else:
            my_db = load_signal(DS2, winL, winR, do_preprocess)

        print("Saving signal processed data ...")
        # Save data
        # Protocol version 0 is the original ASCII protocol and is backwards compatible with earlier versions of Python.
        # Protocol version 1 is the old binary format which is also compatible with earlier versions of Python.
        # Protocol version 2 was introduced in Python 2.3. It provides much more efficient pickling of new-style classes.
        f = open(mit_pickle_name, 'wb')
        pickle.dump(my_db, f, 2)

    features = np.array([], dtype=float)
    labels = np.array([], dtype=np.int32)


    # Compute RR features
    if use_RR or norm_RR:
        if DS == 'DS1':
            RR = [RR_intervals() for i in range(len(DS1))]
        else:
            RR = [RR_intervals() for i in range(len(DS2))]

        print("Computing RR intervals ...")

        for p in range(len(my_db.beat)):
            if maxRR:
                RR[p] = compute_RR_intervals(my_db.R_pos[p])
            else:
                RR[p] = compute_RR_intervals(my_db.orig_R_pos[p])
                
            RR[p].pre_R = RR[p].pre_R[(my_db.valid_R[p] == 1)]
            RR[p].post_R = RR[p].post_R[(my_db.valid_R[p] == 1)]
            RR[p].local_R = RR[p].local_R[(my_db.valid_R[p] == 1)]
            RR[p].global_R = RR[p].global_R[(my_db.valid_R[p] == 1)]


    if use_RR:
        f_RR = np.empty((0,4))
        for p in range(len(RR)):
            row = np.column_stack((RR[p].pre_R, RR[p].post_R, RR[p].local_R, RR[p].global_R))
            f_RR = np.vstack((f_RR, row))

        features = np.column_stack((features, f_RR)) if features.size else f_RR
    
    if norm_RR:
        f_RR_norm = np.empty((0,4))
        for p in range(len(RR)):
            # Compute avg values!
            avg_pre_R = np.average(RR[p].pre_R)
            avg_post_R = np.average(RR[p].post_R)
            avg_local_R = np.average(RR[p].local_R)
            avg_global_R = np.average(RR[p].global_R)

            row = np.column_stack((RR[p].pre_R / avg_pre_R, RR[p].post_R / avg_post_R, RR[p].local_R / avg_local_R, RR[p].global_R / avg_global_R))
            f_RR_norm = np.vstack((f_RR_norm, row))

        features = np.column_stack((features, f_RR_norm))  if features.size else f_RR_norm

    #########################################################################################
    # Compute morphological features
    print("Computing morphological features (" + DS + ") ...")

    # Wavelets
    if 'wavelets' in compute_morph:
        print("Wavelets ...")
        f_wav = np.empty((0,23))

        for p in range(len(my_db.beat)):
            for b in my_db.beat[p]:
                f_wav = np.vstack((f_wav, compute_wavelet_descriptor(b, 'db1', 3)))

        features = np.column_stack((features, f_wav))  if features.size else f_wav

    # HOS
    if 'HOS' in compute_morph:
        print("HOS ...")
        n_intervals = 6
        lag = int(round( (winL + winR )/ n_intervals))

        f_HOS = np.empty((0,10))
        for p in range(len(my_db.beat)):
            for b in my_db.beat[p]:
                f_HOS = np.vstack((f_HOS, compute_hos_descriptor(b, n_intervals, lag)))

        features = np.column_stack((features, f_HOS))  if features.size else f_HOS

    # My morphological descriptor
    if 'myMorph' in compute_morph:
        print("My Descriptor ...")

        f_myMorhp = np.empty((0,4))
        for p in range(len(my_db.beat)):
            for b in my_db.beat[p]:
                f_myMorhp = np.vstack((f_myMorhp, compute_my_own_descriptor(b, winL, winR)))
                   
        features = np.column_stack((features, f_myMorhp))  if features.size else f_myMorhp

    
    labels = np.array(sum(my_db.class_ID, [])).flatten()
    print("labels")
    # Set labels array!

    # TODO Save as: ?
    # input_data = np.column_stack((features, labels))
    # feature_00, feature_0M, labels_0
    # ....
    # feature_N0, feature_NM, labels_N
    
    # Return features and labels
    return features, labels



# DS: contains the patient list for load
# winL, winR: indicates the size of the window centred at R-peak at left and right side
# do_preprocess: indicates if preprocesing of remove baseline on signal is performed
def load_signal(DS, winL, winR, do_preprocess):

    class_ID = [[] for i in range(len(DS))]
    beat = [[] for i in range(len(DS))]
    R_poses = [ np.array([]) for i in range(len(DS))]
    Original_R_poses = [ np.array([]) for i in range(len(DS))]   
    valid_R = [ np.array([]) for i in range(len(DS))]
    my_db = mit_db()
    patients = []

    # Lists 
    # beats = []
    # classes = []
    # valid_R = np.empty([])
    # R_poses = np.empty([])
    # Original_R_poses = np.empty([])

    size_RR_max = 20

    pathDB = '/home/mondejar/dataset/ECG/'
    DB_name = 'mitdb'
    fs = 360
    jump_lines = 1

    # Read files: signal (.csv )  annotations (.txt)    
    fRecords = list()
    fAnnotations = list()

    lst = os.listdir(pathDB + DB_name + "/csv")
    lst.sort()
    for file in lst:
        if file.endswith(".csv"):
            if int(file[0:3]) in DS:
                fRecords.append(file)
        elif file.endswith(".txt"):
            if int(file[0:3]) in DS:
                fAnnotations.append(file)        

    MITBIH_classes = ['N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F']#, 'P', '/', 'f', 'u']
    AAMI_classes = []
    AAMI_classes.append(['N', 'L', 'R'])                    # N
    AAMI_classes.append(['A', 'a', 'J', 'S', 'e', 'j'])     # SVEB 
    AAMI_classes.append(['V', 'E'])                         # VEB
    AAMI_classes.append(['F'])                              # F
    #AAMI_classes.append(['P', '/', 'f', 'u'])              # Q

    RAW_signals = []
    r_index = 0

    #for r, a in zip(fRecords, fAnnotations):
    for r in range(0, len(fRecords)):

        print("Processing signal " + str(r) + " / " + str(len(fRecords)) + "...")

        # 1. Read signalR_poses
        filename = pathDB + DB_name + "/csv/" + fRecords[r]
        print filename
        f = open(filename, 'rb')
        reader = csv.reader(f, delimiter=',')
        next(reader) # skip first line!
        MLII_index = 1
        if int(fRecords[r][0:3]) == 114:
            MLII_index = 2

        MLII = []
        for row in reader:
            MLII.append(int(row[MLII_index]))
            #V1.append(row[2])
        f.close()

        RAW_signals.append(MLII) ## NOTE a copy must be created in order to preserve the original signal
        # display_signal(MLII)

        # 2. Read annotations
        filename = pathDB + DB_name + "/csv/" + fAnnotations[r]
        print filename
        f = open(filename, 'rb')
        next(f) # skip first line!

        annotations = []
        for line in f:
            annotations.append(line)
        f.close
        # 3. Preprocessing signal!
        if do_preprocess:
            #scipy.signal
            # median_filter1D
            baseline = medfilt(MLII, 71) 
            baseline = medfilt(baseline, 215) 

            # Remove Baseline
            for i in range(0, len(MLII)):
                MLII[i] = MLII[i] - baseline[i]

            # TODO Remove High Freqs

        # Extract the R-peaks from annotations
        for a in annotations:
            aS = a.split()
            
            pos = int(aS[1])
            originalPos = int(aS[1])
            classAnttd = aS[2]
            if pos > size_RR_max and pos < (len(MLII) - size_RR_max):
                index, value = max(enumerate(MLII[pos - size_RR_max : pos + size_RR_max]), key=operator.itemgetter(1))
                pos = (pos - size_RR_max) + index

            peak_type = 0
            #pos = pos-1
            
            if classAnttd in MITBIH_classes:
                if(pos > winL and pos < (len(MLII) - winR)):
                    beat[r].append(MLII[pos - winL : pos + winR])
                    classAnttd 
                    for i in range(0,len(AAMI_classes)):
                        if classAnttd in AAMI_classes[i]:
                            class_AAMI = i
                            break #exit loop

                    #conver class
                    class_ID[r].append(class_AAMI)

                    valid_R[r] = np.append(valid_R[r], 1)
                else:
                    valid_R[r] = np.append(valid_R[r], 0)
            else:
                valid_R[r] = np.append(valid_R[r], 0)
            
            R_poses[r] = np.append(R_poses[r], pos)
            Original_R_poses[r] = np.append(Original_R_poses[r], originalPos)
        
        #R_poses[r] = R_poses[r][(valid_R[r] == 1)]
        #Original_R_poses[r] = Original_R_poses[r][(valid_R[r] == 1)]

        
    # Set the data into a bigger struct that keep all the records!
    my_db.filename = fRecords

    my_db.raw_signal = RAW_signals
    my_db.beat = beat
    my_db.class_ID = class_ID
    my_db.valid_R = valid_R
    my_db.R_pos = R_poses
    my_db.orig_R_pos = Original_R_poses

    return my_db
