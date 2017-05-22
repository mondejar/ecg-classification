import numpy as np
import matplotlib.pyplot as pp
import os
import csv
import pickle

dataset_path = '/local/scratch/mondejar/ECG/dataset/mitdb/csv'
window_size = 160

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
        num_recs = num_recs + 1
        records.insert(num_recs, dataset_path + '/' + filename + file_extension)
    else:
        num_annotations = num_annotations +1
        annotation_files.insert(num_annotations, dataset_path + '/' + filename + file_extension)
            
records.sort()
annotation_files.sort()

list_classes = {'N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u'}
# read anotations

for r in records:
    with open(r, 'rb') as csvfile:
        spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
        for row in spamreader:
            print ', '.join(row)

    #data_csv = 
    #signal_II = 


# Save data
#f = open('store.pckl', 'wb')
#pickle.dump(object, f)
#f.close()

# Load data
#f = open('store.pckl', 'rb')
#object = pickle.load(f)
#f.close()


#val = 0. # this is the value where you want the data to appear on the y-axis.
#ar = np.arange(10) # just as an example array
#pp.plot(ar, np.zeros_like(ar) + val, 'x')
#pp.show()

# Create dataset