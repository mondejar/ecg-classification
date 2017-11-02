#!/usr/bin/env python

"""
features_ECG.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
23 Oct 2017
"""


import numpy as np
from scipy.signal import medfilt
import scipy.stats
import pywt
import operator

from mit_db import *

# Input: the R-peaks from a signal
# Return: the features RR intervals 
#   (pre_RR, post_RR, local_RR, global_RR)
#    for each beat 
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
        for j in range(-9, 1):
            if j+i >= 0:
                avg_val = avg_val + pre_R[i+j]
                num = num +1
        local_R = np.append(local_R, avg_val / float(num))

	# Global R AVG: from full past-signal
    # TODO: AVG from past 5 minutes = 108000 samples
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

# Compute the wavelet descriptor for a beat
def compute_wavelet_descriptor(beat, family, level):
    db1 = pywt.Wavelet(family)
    coeffs = pywt.wavedec(beat, db1, level=level)
    return coeffs[0]

# Compute the HOS descriptor for a beat
# Skewness (3 cumulant) and kurtosis (4 cumulant)
def compute_host_descriptor(beat, n_intervals, lag):
    hos_b = np.zeros((10))
    for i in range(0, n_intervals-1):
        pose = (lag * i)
        interval = beat[pose:(pose + lag - 1)]
        # Skewness  
        hos_b[i] = scipy.stats.skew(interval, 0, True)
        # Kurtosis
        hos_b[5+i] = scipy.stats.kurtosis(interval, 0, False, True)
    return hos_b

# Compute my descriptor based on amplitudes of several intervals
def compute_my_own_descriptor(beat, winL, winR):
    R_pos = int((winL + winR) / 2)

    R_value = beat[R_pos]
    my_morph = np.zeros((4))
    y_values = np.zeros(4)
    x_values = np.zeros(4)
    # Obtain (max/min) values and index from the intervals
    [x_values[0], y_values[0]] = max(enumerate(beat[0:40]), key=operator.itemgetter(1))
    [x_values[1], y_values[1]] = min(enumerate(beat[75:85]), key=operator.itemgetter(1))
    [x_values[2], y_values[2]] = min(enumerate(beat[95:105]), key=operator.itemgetter(1))
    [x_values[3], y_values[3]] = max(enumerate(beat[150:180]), key=operator.itemgetter(1))
    
    x_values[1] = x_values[1] + 75
    x_values[2] = x_values[2] + 95
    x_values[3] = x_values[3] + 150
    
    # Norm data before compute distance
    x_max = max(x_values)
    y_max = max(np.append(y_values, R_value))
    x_min = min(x_values)
    y_min = min(np.append(y_values, R_value))
    
    R_pos = (R_pos - x_min) / (x_max - x_min)
    R_value = (R_value - y_min) / (y_max - y_min)
                
    for n in range(0,4):
        x_values[n] = (x_values[n] - x_min) / (x_max - x_min)
        y_values[n] = (y_values[n] - y_min) / (y_max - y_min)
        x_diff = (R_pos - x_values[n]) 
        y_diff = R_value - y_values[n]
        my_morph[n] =  np.linalg.norm([x_diff, y_diff])
        # TODO test with np.sqrt(np.dot(x_diff, y_diff))
    
    if np.isnan(my_morph[n]):
        my_morph[n] = 0.0

    return my_morph