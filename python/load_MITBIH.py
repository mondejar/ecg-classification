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
import matplotlib.pyplot as plt
from scipy.signal import medfilt
import numpy as np

from mit_db import *

# Following the inter-patient scheme division by [] DS1 / DS2
def main(args):
    print("Main example of load_MITBIH database")

    DS1 = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230]
    DS2 = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234]

    do_preprocess = True
    maxRR = True

    winL = 90
    winR = 90

    load_signal(DS1, winL, winR, do_preprocess, maxRR)

# Compute features RR interval for each  beat
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
            MLII.append(float(row[1]))
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
            # median_filter1D
            baseline = medfilt(MLII, 71) #scipy.signal
            baseline = medfilt(baseline, 215) #scipy.signal

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

if __name__ == '__main__':
    import sys
    main(sys.argv[1:])