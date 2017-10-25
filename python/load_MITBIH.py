#!/usr/bin/env python

"""
load_MITBIH.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
23 Oct 2017
"""

import os
import csv
import operator
import gc
import cPickle as pickle

import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import medfilt
import scipy.stats
import pywt

from mit_db import *

# Load the data with the configuration and features selected
def load_mit_db(DS, winL, winR, do_preprocess, maxRR, use_RR, norm_RR, compute_morph):
    print("Loading MIT BIH arr (" + DS + ") ...")

    DS1 = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230]
    DS2 = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234]

    db_path = '/home/mondejar/dataset/ECG/mitdb/'

    mit_pickle_name = db_path + 'm_learning/python_mit'
    if do_preprocess:
        mit_pickle_name = mit_pickle_name + '_rm_bsline'
    if maxRR:
        mit_pickle_name = mit_pickle_name + '_max_RR'
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
            my_db = load_signal(DS1, winL, winR, do_preprocess, maxRR)
        else:
            my_db = load_signal(DS2, winL, winR, do_preprocess, maxRR)

        print("Saving signal processed data ...")
        # Save data
        # Protocol version 0 is the original ASCII protocol and is backwards compatible with earlier versions of Python.
        # Protocol version 1 is the old binary format which is also compatible with earlier versions of Python.
        # Protocol version 2 was introduced in Python 2.3. It provides much more efficient pickling of new-style classes.
        f = open(mit_pickle_name, 'wb')
        pickle.dump(my_db, f, 2)

    features = np.array([], dtype=float)
    labels = np.array([], dtype=np.int32)

    # RR features
    if use_RR:
        f_RR = np.empty((0,4))
        for p in range(len(my_db.RR)):
            row = np.column_stack((my_db.RR[p].pre_R, my_db.RR[p].post_R, my_db.RR[p].local_R, my_db.RR[p].global_R))
            f_RR = np.vstack((f_RR, row))

        features = np.column_stack((features, f_RR))  if features.size else f_RR
    
    if norm_RR:
        f_RR_norm = np.empty((0,4))
        for p in range(len(my_db.RR)):
            # Compute avg values!
            avg_pre_R = np.average(my_db.RR[p].pre_R)
            avg_post_R = np.average(my_db.RR[p].post_R)
            avg_local_R = np.average(my_db.RR[p].local_R)
            avg_global_R = np.average(my_db.RR[p].global_R)

            row = np.column_stack((my_db.RR[p].pre_R / avg_pre_R, my_db.RR[p].post_R / avg_post_R, my_db.RR[p].local_R / avg_local_R, my_db.RR[p].global_R / avg_global_R))
            f_RR_norm = np.vstack((f_RR_norm, row))

        features = np.column_stack((features, f_RR_norm))  if features.size else f_RR_norm

    #########################################################################################
    # Compute morphological features
    print("Computing morphological features (" + DS + ") ...")

    # Wavelets
    if 'wavelets' in compute_morph:
        f_wav = np.empty((0,23))

        for p in range(len(my_db.beat)):
            for b in my_db.beat[p]:
                db1 = pywt.Wavelet('db1')
                coeffs = pywt.wavedec(b, db1, level=3)
                wav = coeffs[0]
                
                f_wav = np.vstack((f_wav, wav))

        features = np.column_stack((features, f_wav))  if features.size else f_wav

    
    # HOS
    if 'HOS' in compute_morph:
        n_intervals = 6
        lag = int(round( (winL + winR )/ n_intervals))

        f_HOS = np.empty((0,10))

        for p in range(len(my_db.beat)):
            for b in my_db.beat[p]:
                hos_b = np.empty((0,10))
                for i in range(0, n_intervals-1):
                    pose = (lag * i)
                    interval = b[pose:(pose + lag - 1)]
                    # Skewness  
                    hos_b[i] = scipy.stats.skew(interval, 0, True)
                    # Kurtosis
                    hos_b[5+i] = scipy.stats.kurtosis(interval, 0, False, True)
               
                f_HOS = np.vstack((f_HOS, hos_b))
        features = np.column_stack((features, f_HOS))  if features.size else f_HOS

    # My morphological descriptor
    #if 'myMorph' in compute_morph:


        #features = np.column_stack((features, f_myMorhp))  if features.size else f_myMorhp


    # Set labels array!

    # TODO Save as: ?
    # feature_00, feature_0M, labels_0
    # ....
    # feature_N0, feature_NM, labels_N
    
    # Return features and labels
    return features, labels




# Compute features RR interval for each beat 
def compute_RR_intervals(R_poses):
    features_RR = RR_intervals()

    pre_R = np.array([], dtype=int)
    post_R = np.array([], dtype=int)
    local_R = np.array([], dtype=int)
    global_R = np.array([], dtype=int)

    # Pre_R and Post_R
    pre_R = np.append(pre_R, 0)
    post_R = np.append(post_R, R_poses[1] -R_poses[0])

    for i in range(1, len(R_poses)-1):
        pre_R = np.append(pre_R, R_poses[i] - R_poses[i-1])
        post_R = np.append(post_R, R_poses[i+1] - R_poses[i])

    pre_R[0] = pre_R[1]
    pre_R = np.append(pre_R, R_poses[-1] - R_poses[-2])  

    post_R = np.append(post_R, post_R[-1])

    # Local_R: AVG from last 10 pre_R values
    for i in range(0, len(R_poses)):
        num = 0
        avg_val = 0
        for j in range(-9, 0):
            if j+i >= 0:
                avg_val = avg_val + pre_R[i+j]
                num = num +1
        if num > 0:
            local_R = np.append(local_R, avg_val / float(num))
        else:
            local_R = np.append(local_R, 0.0)

	# Global R AVG: from full past-signal
	global_R = np.append(global_R, pre_R[0])
    for i in range(1, len(R_poses)):
        num = 0
        avg_val = 0
        for j in range( 0, i):
            avg_val = avg_val + pre_R[j]
        num = i
        global_R = np.append(global_R, avg_val / float(num))

    for i in range(0, len(R_poses)):
        features_RR.pre_R = np.append(features_RR.pre_R, pre_R[i])
        features_RR.post_R = np.append(features_RR.post_R, post_R[i])
        features_RR.local_R = np.append(features_RR.local_R, local_R[i])
        features_RR.global_R = np.append(features_RR.global_R, global_R[i])

        #features_RR.append([pre_R[i], post_R[i], local_R[i], global_R[i]])
            
    return features_RR

# DS: contains the patient list for load
# winL, winR: indicates the size of the window centred at R-peak at left and right side
# do_preprocess: indicates if preprocesing of remove baseline on signal is performed
# maxRR: indicates if the beats selected are centred exactly on the max value from R mountain
def load_signal(DS, winL, winR, do_preprocess, maxRR):

    class_ID = [[] for i in range(len(DS))]
    beat = [[] for i in range(len(DS))]
    R_poses = [ np.array([]) for i in range(len(DS))]
    Original_R_poses = [ np.array([]) for i in range(len(DS))]   
    valid_R = [ np.array([]) for i in range(len(DS))]
    features_RR = [ [] for i in range(len(DS))]
    fRR = [RR_intervals() for i in range(len(DS))]
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
            MLII.append(int(row[1]))
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
            if maxRR and pos > size_RR_max and pos < (len(MLII) - size_RR_max):
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
        
        # Compute RR-intervals
        # [pre_R, post_R, local_R, global_R]
        features_RR[r] = compute_RR_intervals(R_poses[r])
        #features_RR[r] = np.asarray(f_RR)
        
        # Only keep information of beats that class annotation is valid (valid_R)
        features_RR[r].pre_R = features_RR[r].pre_R[(valid_R[r] == 1)]
        features_RR[r].post_R = features_RR[r].post_R[(valid_R[r] == 1)]
        features_RR[r].local_R = features_RR[r].local_R[(valid_R[r] == 1)]
        features_RR[r].global_R = features_RR[r].global_R[(valid_R[r] == 1)]

        R_poses[r] = R_poses[r][(valid_R[r] == 1)]
        Original_R_poses[r] = Original_R_poses[r][(valid_R[r] == 1)]

        
    # Set the data into a bigger struct that keep all the records!
    my_db.filename = fRecords

    my_db.raw_signal = RAW_signals
    my_db.beat = beat
    my_db.RR = features_RR

    my_db.class_ID = class_ID
    my_db.valid_R = valid_R
    my_db.R_pos = R_poses
    my_db.orig_R_pos = Original_R_poses
    
    return my_db
