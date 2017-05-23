""" 
Extract beats from mitdb dataset with size = 2 * window_size
and compute temporal features from each beat

Author: Mondejar Guerra
VARPA
University of A Coruna
April 2017
"""

import os
import csv
import pickle
import collections
from collections import namedtuple
import numpy as np
import matplotlib.pyplot as plt

class temp_features:
    def __init__(self):
        # Instance atributes
        self.pre_R = []
        self.post_R = []
        self.local_R = []
        self.global_R = []

class mit_data:
    def __init__(self):
        # Instance atributes
        self.filenames = []
        self.patients = []
        self.signals = []
        self.classes = []   
        self.selected_R = []       
        self.temp_features = []       
        self.window_size = []       
            
dataset_path = '/local/scratch/mondejar/ECG/dataset/mitdb/csv'
window_size = 160
compute_RR_interval_feature = True

list_classes = ['N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u']

# read files
filenames = next(os.walk(dataset_path))[2]

# .csv
num_recs = 0
num_annotations = 0

records = []
annotation_files = []
filenames.sort()

for f in filenames:
    filename, file_extension = os.path.splitext(f)
    if(file_extension == '.csv'):
        records.insert(num_recs, dataset_path + '/' + filename + file_extension)
        num_recs = num_recs + 1
    else:
        annotation_files.insert(num_annotations, dataset_path + '/' + filename + file_extension)
        num_annotations = num_annotations +1

signal_II_w = [ np.array([np.array([])]) for i in range(len(records))]
classes = [[] for i in range(len(records))]
R_poses = [[] for i in range(len(records))]
selected_R = [np.array([]) for i in range(len(records))]
temporal_features = [temp_features for i in range(len(records))]
mit_db = mit_data()

r_index = 0
files = []
patients = []
for r in range(0,len(records),1):
    print(r)
    signal_II = []
    csvfile = open(records[r], 'rb')
    spamreader = csv.reader(csvfile, delimiter=',', quotechar='|')
    row_index = -1
    for row in spamreader:
        if(row_index >= 0):
            signal_II.insert(row_index, int(row[1]))
        row_index = row_index +1

    # Display signal II 
    #plt.plot(signal_II)
    #plt.show()
    patients.append(records[r][-7:-4])

    # read anotations: R position and class
    fileID = open(annotation_files[r], 'r')
    data = fileID.readlines() 
    beat = 0
    for d in range(1, len(data), 1):
        splitted = data[d].split(' ')
        splitted = filter(None, splitted) 
        pos = int(splitted[1]) 
        type = splitted[2]
        if(type in list_classes):
            if(pos > window_size and pos < (len(signal_II) - window_size)):
                beat = signal_II[pos-window_size+1:pos+window_size]
                if np.size(signal_II_w[r]) == 0:
                    signal_II_w[r] = beat
                else:
                    signal_II_w[r] = np.vstack([signal_II_w[r], beat])
                       
                classes[r].append(type)
                selected_R[r] = np.append(selected_R[r], 1)
            else:
                selected_R[r] = np.append(selected_R[r], 0)
        else:
            selected_R[r] = np.append(selected_R[r], 0)
        R_poses[r].append(pos)

    # Compute RR Interval, feature Time
    if(compute_RR_interval_feature):

        pre_R = np.array([0])
        post_R = np.array([R_poses[r][1] - R_poses[r][0]])
        local_R = np.array([]) # Average of the ten past R intervals
        global_R = np.array([]) # Average of the last 5 minutes of the signal
        
        for i in range(1,len(R_poses[r])-1, 1):
            pre_R = np.insert(pre_R, i, R_poses[r][i] - R_poses[r][i-1])
            post_R = np.insert(post_R, i, R_poses[r][i+1] - R_poses[r][i])
        
        pre_R[0] = pre_R[1]
        pre_R = np.append(pre_R, R_poses[r][-1] - R_poses[r][-2])
        post_R = np.append(post_R, post_R[-1])
        
        # Local R: AVG from past 10 RR intervals
        for i in range(0,len(R_poses[r]), 1):
            avg_val = 0
            num_elems = 0
            window = range(i-10,i,1)
            
            for w in window:
                if w >= 0:
                    avg_val = avg_val + pre_R[w]
                    num_elems = num_elems + 1

            if num_elems == 0:
                local_R = np.append(local_R, 0)   
            else:
                avg_val = avg_val / num_elems
                local_R = np.append(local_R, avg_val)   
        
        # Global R: AVG from past 5 minutes
        # 360 Hz  5 minutes = 108000 samples;
        for i in range(0, len(R_poses[r]), 1):
            avg_val = 0
            back = -1
            back_length = 0
            if R_poses[r][i] < 108000:
                window = range(0,i,1)
            else:
                while (i + back) > 0 and back_length < 108000:
                    back_length = R_poses[r][i] - R_poses[r][i+back]
                    back = back -1
                window = range(max(0,(back+i)), i, 1)
            # Considerando distancia maxima hacia atras
            for w in window:
                avg_val = avg_val + pre_R[w]

            if len(window) > 0:
                avg_val = avg_val / len(window)
            else:
                avg_val = 0
            global_R= np.append(global_R, avg_val)
        
        # Only keep those features from beats that we save list_classes
        # but for the computation of temporal features all the beats must be used
        temporal_features[r].pre_R = pre_R[np.where(selected_R[r] == 1)[0]]
        temporal_features[r].post_R = post_R[np.where(selected_R[r] == 1)[0]]
        temporal_features[r].local_R = local_R[np.where(selected_R[r] == 1)[0]]
        temporal_features[r].global_R = global_R[np.where(selected_R[r] == 1)[0]]

# EXPORT
mit_db.filenames = records
mit_db.patients = patients
mit_db.signals = signal_II_w
mit_db.classes = classes
mit_db.selected_R = selected_R
mit_db.temporal_features = temporal_features
mit_db.window_size = window_size

# Save data
output_path = '/local/scratch/mondejar/ECG/dataset/mitdb/m_learning/'
# Protocol version 0 is the original ASCII protocol and is backwards compatible with earlier versions of Python.
# Protocol version 1 is the old binary format which is also compatible with earlier versions of Python.
# Protocol version 2 was introduced in Python 2.3. It provides much more efficient pickling of new-style classes.
pickle.dump(mit_db, open(output_path + 'mit_db_' + str(window_size) + '.p', 'wb'), 2)

# Load data
#mit_db  = pickle.load(open('mit_db.p', 'rb'))
