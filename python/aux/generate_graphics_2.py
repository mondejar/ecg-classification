import matplotlib.pyplot as plt
import numpy as np
from features_ECG import *
import os
import csv
import gc
import cPickle as pickle

import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import medfilt
import scipy.stats
import pywt
import time
import sklearn
from sklearn import decomposition
from sklearn.decomposition import PCA, IncrementalPCA

from features_ECG import *


# DS: contains the patient list for load
# winL, winR: indicates the size of the window centred at R-peak at left and right side
# do_preprocess: indicates if preprocesing of remove baseline on signal is performed

DS = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230]
winL = 90
winR = 90
do_preprocess = True
class_ID = [[] for i in range(len(DS))]
beat = [[] for i in range(len(DS))] # record, beat, lead
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
#for r in range(0, len(fRecords)):
r = 4
print("Processing signal " + str(r) + " / " + str(len(fRecords)) + "...")

# 1. Read signalR_poses
filename = pathDB + DB_name + "/csv/" + fRecords[r]
print filename
f = open(filename, 'rb')
reader = csv.reader(f, delimiter=',')
next(reader) # skip first line!
MLII_index = 1
V1_index = 2
if int(fRecords[r][0:3]) == 114:
    MLII_index = 2
    V1_index = 1

MLII = []
V1 = []
for row in reader:
    MLII.append((int(row[MLII_index])))
    V1.append((int(row[V1_index])))
f.close()


RAW_signals.append((MLII, V1)) ## NOTE a copy must be created in order to preserve the original signal
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



# 1 Display signal
##############################################################################
plt.figure(figsize=(11.69, 8.27))
ax1 = plt.subplot(311)
plt.plot(MLII[22500:24500])


# 3. Preprocessing signal!
baseline = medfilt(MLII, 71) 
baseline = medfilt(baseline, 215) 

# Remove Baseline
for i in range(0, len(MLII)):
    MLII[i] = MLII[i] - baseline[i]
# TODO Remove High Freqs



for i in range(0, len(MLII)):
    MLII[i] = MLII[i] - baseline[i]
    
# 2 Remove Noise
##############################################################################
ax2 = plt.subplot(312)
plt.plot(MLII[22500:24500])


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
            beat[r].append( (MLII[pos - winL : pos + winR], V1[pos - winL : pos + winR]))
            for i in range(0,len(AAMI_classes)):
                if classAnttd in AAMI_classes[i]:
                    class_AAMI = i
                    break #exit loop
            #convert class
            class_ID[r].append(class_AAMI)

            valid_R[r] = np.append(valid_R[r], 1)
        else:
            valid_R[r] = np.append(valid_R[r], 0)
    else:
        valid_R[r] = np.append(valid_R[r], 0)
    
    R_poses[r] = np.append(R_poses[r], pos)
    Original_R_poses[r] = np.append(Original_R_poses[r], originalPos)


# 3 Mark Fiducial Points 
##############################################################################
ax3 = plt.subplot(313)
plt.plot(MLII[22500:24500])

for pose in R_poses[r]:
    if pose > 22500 and pose < 24500:
        circle = plt.Circle((pose - 22500, MLII[int(pose)]), radius= 10, color='g')
        ax3.add_patch(circle)

#plt.show()

plt.savefig('/home/mondejar/graphic_2.pdf', dpi=None, facecolor='w', edgecolor='w',
    orientation='landscape', papertype='a4', format='pdf', transparent=True, bbox_inches=None, 
    pad_inches=0.1, frameon=None)