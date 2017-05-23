import numpy as np
import matplotlib.pyplot as plt
import os
import csv
import pickle

dataset_path = '/local/scratch/mondejar/ECG/dataset/mitdb/csv'
window_size = 160
compute_RR_interval_feature = True

# read files
filenames = next(os.walk(dataset_path))[2]

# .csv
num_recs = 0
num_annotations = 0

records = []
annotation_files = []

for f in filenames:
    filename, file_extension = os.path.splitext(f)
    if(file_extension == '.csv'):
        records.insert(num_recs, dataset_path + '/' + filename + file_extension)
        num_recs = num_recs + 1
    else:
        annotation_files.insert(num_annotations, dataset_path + '/' + filename + file_extension)
        num_annotations = num_annotations +1
            
records.sort()
annotation_files.sort()

list_classes = ['N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u']

# signal_II = [[] for i in range(len(records))]
signal_II_w = [[] for i in range(len(records))]
classes = [[] for i in range(len(records))]
selected_R = [[] for i in range(len(records))]
R_poses = [[] for i in range(len(records))]

r_index = 0
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
                #signals{r} = [signals{r} data1(pos-window_size+1: pos+ window_size)];
                signal_II_w[r] = signal_II[pos-window_size+1:pos+window_size]
                # plt.plot(signal_II_w[r])
                # plt.show()
                classes[r].append(type)
                selected_R[r].append(1)
            else:
                selected_R[r].append(0)
        else:
            selected_R[r].append(0)
        
        R_poses[r].append(pos)

    # Compute RR Interval, feature Time
    if(compute_RR_interval_feature):
        pre_R = [0]
        post_R = [R_poses[r][1] - R_poses[r][0]]
        local_R = [] # Average of the ten past R intervals
        global_R = [] # Average of the last 5 minutes of the signal
        
        for i in range(1,len(R_poses[r])-1, 1):
            pre_R.insert(i, R_poses[r][i] - R_poses[r][i-1])
            post_R.insert(i, R_poses[r][i+1] - R_poses[r][i])
        
        pre_R[0] = pre_R[1]
        pre_R.append(R_poses[r][-1] - R_poses[r][-2])
        post_R.append(post_R[-1])
        
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
                local_R.append(0)   
            else:
                avg_val = avg_val / num_elems
                local_R.append(avg_val)   
        
            # Global R: AVG from past 5 minutes
            # 360 Hz  5 minutes = 108000 samples;
            for i in range(1, len(R_poses[r]), 1):
                avg_val = 0
                back = -1
                back_length = 0
                if R_poses[r][i] < 108000:
                    window = range(1,i,1)
                else:
                    while (i + back) > 0 and back_length < 108000:
                        back_length = R_poses[r][i] - R_poses[r][i+back]
                        back = back -1
                    window = range(max(1,(back+i)), i, 1)
                # Considerando distancia maxima hacia atras 
                for w in window:
                    avg_val = avg_val + pre_R[w]

                avg_val = avg_val / len(window)
                global_R.append(avg_val)
        
    #    %% Only keep those features from beats that we save list_classes
    #    %% but for the computation of temporal features all the beats must be used
    #    temporal_features{r}.pre_R = pre_R(selected_R{r} == 1);
    #    temporal_features{r}.post_R = post_R(selected_R{r} == 1);
    #    temporal_features{r}.local_R = local_R(selected_R{r} == 1);
    #    temporal_features{r}.global_R = global_R(selected_R{r} == 1);
 
    # data_csv = 
    # signal_II = 

# Save data
#f = open('store.pckl', 'wb')
#pickle.dump(object, f)
#f.close()

# Load data
#f = open('store.pckl', 'rb')
#object = pickle.load(f)
#f.close()

# Create dataset