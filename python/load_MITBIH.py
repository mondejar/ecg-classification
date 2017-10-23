#!/usr/bin/env python

"""
load_MITBIH.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
23 Oct 2017
"""

# # Following the inter-patient scheme division by [] DS1 / DS2
# DS1 = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230]
# DS2 = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234]

# DS: contains the patient list for load
def load_signal(DS):

    pathDB = '/home/mondejar/dataset/ECG/'
    DB_name = 'mitdb'
    fs = 360
    jump_lines = 1

    # Read files: signal (.csv )  annotations (.txt)