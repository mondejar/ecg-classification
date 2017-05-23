""" 

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

window_size = 160

db_path = '/local/scratch/mondejar/ECG/dataset/mitdb/m_learning/'

# Load data
mit_db  = pickle.load(open(db_path + 'mit_db_' + str(window_size) + '.p', 'rb'))
