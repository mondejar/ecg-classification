#!/usr/bin/env python

"""
mit_db.py

Description:
Contains the classes for store the MITBIH database and some utils

VARPA, University of Coruna
Mondejar Guerra, Victor M.
24 Oct 2017
"""

import matplotlib.pyplot as plt
import numpy as np

# Show a 2D plot with the data in beat
def display_signal(beat):
    plt.plot(beat)
    plt.ylabel('Signal')
    plt.show()

# Class for RR intervals features
class RR_intervals:
    def __init__(self):
        # Instance atributes
        self.pre_R = np.array([])
        self.post_R = np.array([])
        self.local_R = np.array([])
        self.global_R = np.array([])
        
class mit_db:
    def __init__(self):
        # Instance atributes
        self.filename = []
        self.raw_signal = []
        self.beat = np.empty([]) # record, beat, lead
        self.class_ID = []   
        self.valid_R = []       
        self.R_pos = []
        self.orig_R_pos = []
