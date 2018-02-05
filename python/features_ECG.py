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
    post_R = np.append(post_R, R_poses[1] - R_poses[0])

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
            if (R_poses[i] - R_poses[j]) < 108000:
                avg_val = avg_val + pre_R[j]
                num = num + 1
        #num = i
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
    wave_family = pywt.Wavelet(family)
    coeffs = pywt.wavedec(beat, wave_family, level=level)
    return coeffs[0]

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


# Compute the HOS descriptor for a beat
# Skewness (3 cumulant) and kurtosis (4 cumulant)
def compute_hos_descriptor(beat, n_intervals, lag):
    hos_b = np.zeros(( (n_intervals-1) * 2))
    for i in range(0, n_intervals-1):
        pose = (lag * (i+1))
        interval = beat[(pose -(lag/2) ):(pose + (lag/2))]
        
        # Skewness  
        hos_b[i] = scipy.stats.skew(interval, 0, True)

        if np.isnan(hos_b[i]):
            hos_b[i] = 0.0
            
        # Kurtosis
        hos_b[(n_intervals-1) +i] = scipy.stats.kurtosis(interval, 0, False, True)
        if np.isnan(hos_b[(n_intervals-1) +i]):
            hos_b[(n_intervals-1) +i] = 0.0
    return hos_b


uniform_pattern_list = np.array([0, 1, 2, 3, 4, 6, 7, 8, 12, 14, 15, 16, 24, 28, 30, 31, 32, 48, 56, 60, 62, 63, 64, 96, 112, 120, 124, 126, 127, 128,
     129, 131, 135, 143, 159, 191, 192, 193, 195, 199, 207, 223, 224, 225, 227, 231, 239, 240, 241, 243, 247, 248, 249, 251, 252, 253, 254, 255])
# Compute the uniform LBP 1D from signal with neigh equal to number of neighbours
# and return the 59 histogram:
# 0-57: uniform patterns
# 58: the non uniform pattern
# NOTE: this method only works with neigh = 8
def compute_Uniform_LBP(signal, neigh=8):
    hist_u_lbp = np.zeros(59, dtype=float)

    avg_win_size = 2
    # NOTE: Reduce sampling by half
    #signal_avg = scipy.signal.resample(signal, len(signal) / avg_win_size)

    for i in range(neigh/2, len(signal) - neigh/2):
        pattern = np.zeros(neigh)
        ind = 0
        for n in range(-neigh/2,0) + range(1,neigh/2+1):
            if signal[i] > signal[i+n]:
                pattern[ind] = 1          
            ind += 1
        # Convert pattern to id-int 0-255 (for neigh == 8)
        pattern_id = int("".join(str(c) for c in pattern.astype(int)), 2)

        # Convert id to uniform LBP id 0-57 (uniform LBP)  58: (non uniform LBP)
        if pattern_id in uniform_pattern_list:
            pattern_uniform_id = int(np.argwhere(uniform_pattern_list == pattern_id))
        else:
            pattern_uniform_id = 58 # Non uniforms patternsuse

        hist_u_lbp[pattern_uniform_id] += 1.0

    return hist_u_lbp


def compute_LBP(signal, neigh=4):
    hist_u_lbp = np.zeros(np.power(2, neigh), dtype=float)

    avg_win_size = 2
    # TODO: use some type of average of the data instead the full signal...
    # Average window-5 of the signal?
    #signal_avg = average_signal(signal, avg_win_size)
    signal_avg = scipy.signal.resample(signal, len(signal) / avg_win_size)

    for i in range(neigh/2, len(signal) - neigh/2):
        pattern = np.zeros(neigh)
        ind = 0
        for n in range(-neigh/2,0) + range(1,neigh/2+1):
            if signal[i] > signal[i+n]:
                pattern[ind] = 1          
            ind += 1
        # Convert pattern to id-int 0-255 (for neigh == 8)
        pattern_id = int("".join(str(c) for c in pattern.astype(int)), 2)

        hist_u_lbp[pattern_id] += 1.0

    return hist_u_lbp



# https://docs.scipy.org/doc/numpy-1.13.0/reference/routines.polynomials.hermite.html
# Support Vector Machine-Based Expert System for Reliable Heartbeat Recognition
# 15 hermite coefficients!
def compute_HBF(beat):

    coeffs_hbf = np.zeros(15, dtype=float)
    coeffs_HBF_3 = hermfit(range(0,len(beat)), beat, 3) # 3, 4, 5, 6?
    coeffs_HBF_4 = hermfit(range(0,len(beat)), beat, 4)
    coeffs_HBF_5 = hermfit(range(0,len(beat)), beat, 5)
    #coeffs_HBF_6 = hermfit(range(0,len(beat)), beat, 6)

    coeffs_hbf = np.concatenate((coeffs_HBF_3, coeffs_HBF_4, coeffs_HBF_5))

    return coeffs_hbf