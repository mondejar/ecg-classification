""" 
Extract beats from mitdb dataset with size = 2 * window_size
and compute temporal features from each beat

Author: Mondejar Guerra
VARPA
University of A Coruna
April 2017
"""

import numpy as np
import matplotlib.pyplot as plt
import os
import csv
import pickle
import numpy as np
import matplotlib.pyplot as plt
import os.path
import pywt

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
        self.temporal_features = []       
        self.window_size = []       
     
# Function that given a list of patient extract the signals and 
# perform the wavelet descomposition (optionally) and return
# the data and label prepaed for future classification tasks
def get_data_label_mitdb( list_patient, mit_db ):
    labels = np.array([], dtype=np.int32)
    data = np.array([], dtype=float)

    for p in list_patient:
        index = mit_db.patients.index(str(p))
        for b in range(0, len(mit_db.classes[index]), 1):
            RR = [mit_db.temporal_features[index].pre_R[b], mit_db.temporal_features[index].post_R[b], mit_db.temporal_features[index].local_R[b], mit_db.temporal_features[index].global_R[b]]
            signal = mit_db.signals[index][b]
            beat_type = mit_db.classes[index][b]
            # Name class by numbers np.int32
            #['N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u']
            for i in range(0,5,1):
                if beat_type in superclass[i]:
                    class_n = i
                    break #exit loop
            
            labels = np.append(labels, class_n)
            
            #Display raw and wave signal
            if not compute_wavelets:
                features = signal

            else: # db of order 8 
                db8 = pywt.Wavelet('db8')
                coeffs = pywt.wavedec(signal, db8, level=4)
                features = coeffs[1]

            #TODO add RR interval
            if compute_RR_interval_feature:
                features = np.append(features, RR)

            if len(data) == 0:
                data = features
            else:
                data = np.vstack((data, features))           
        
                #plt.subplot(211)
                #plt.plot(signal)
                #plt.subplot(212)
                #plt.plot(coeffs[1])
                #plt.show()
    return (data, labels)

dataset = '/home/mondejar/dataset/ECG/mitdb/'
output_path = dataset + 'm_learning/'
window_size = 160
compute_RR_interval_feature = True
compute_wavelets = True

list_classes = ['N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u']
superclass = []
superclass.append(['N', 'L', 'R', 'e', 'j'])   # N
superclass.append(['A', 'a', 'J', 'S'])        # SVEB 
superclass.append(['V', 'E'])                  # VEB
superclass.append(['F'])                       # F
superclass.append(['P', '/', 'f', 'u'])        # Q

if not os.path.exists(output_path + 'mit_db_' + str(window_size) + '.p'):
    # read files
    filenames = next(os.walk(dataset + 'csv'))[2]

    # .csv
    num_recs = 0
    num_annotations = 0

    records = []
    annotation_files = []
    filenames.sort()

    for f in filenames:
        filename, file_extension = os.path.splitext(f)
        if(file_extension == '.csv'):
            records.insert(num_recs, dataset + 'csv/' + filename + file_extension)
            num_recs = num_recs + 1
        else:
            annotation_files.insert(num_annotations, dataset + 'csv/' + filename + file_extension)
            num_annotations = num_annotations +1

    signal_II_w = [ np.array([np.array([])]) for i in range(len(records))]
    classes = [[] for i in range(len(records))]
    R_poses = [[] for i in range(len(records))]
    selected_R = [np.array([]) for i in range(len(records))]
    temporal_features = [temp_features() for i in range(len(records))]
    mit_db = mit_data()

    r_index = 0
    files = []
    patients = []

    for r in range(0,len(records),1):
        signal_II = []
        print(r)
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

        # read anotations
        fileID = open(annotation_files[r], 'r')
        data = fileID.readlines() 

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
                        signamit_pickle_namel_II_w[r] = np.vstack([signal_II_w[r], beat])
                        
                    classes[r].append(type)
                    selected_R[r] = np.append(selected_R[r], 1)
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
    # Protocol version 0 is the original ASCII protocol and is backwards compatible with earlier versions of Python.
    # Protocol version 1 is the old binary format which is also compatible with earlier versions of Python.
    # Protocol version 2 was introduced in Python 2.3. It provides much more efficient pickling of new-style classes.
    pickle.dump(mit_db, open(output_path + 'mit_db_' + str(window_size) + '.p', 'wb'), 2)
else:
    # Load data
    mit_db  = pickle.load(open(output_path + 'mit_db_' + str(window_size) + '.p', 'rb'))

# Select data for training
list_train_pat = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230]
list_test_pat = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234]

#TODO export data and label directly like Tensorflow would require     
train_data, train_labels = get_data_label_mitdb(list_train_pat, mit_db)
eval_data, eval_labels = get_data_label_mitdb(list_test_pat, mit_db)

extension = '_' + str(window_size)
if compute_wavelets:
    extension = extension + '_' + 'wv'
if compute_RR_interval_feature:
    extension = extension + '_' + 'RR'
extension = extension + '.csv'

# Export, save
np.savetxt(output_path + 'train_data' + extension, train_data, delimiter=",")
np.savetxt(output_path + 'train_label' + extension, train_labels, delimiter=",")
np.savetxt(output_path + 'eval_data' + extension, eval_data, delimiter=",")
np.savetxt(output_path + 'eval_label' + extension, eval_labels, delimiter=",")
