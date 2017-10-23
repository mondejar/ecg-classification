from __future__ import print_function
from pyds import MassFunction
from itertools import product

import sys
import csv
import matplotlib.pyplot as plt
import operator

"""
Read signal from text file
"""
def read_ev_dist( filename ):
    # Read data from file .csv 
    ev_dist = list()
    with open(filename, 'rb') as f:
        reader = csv.reader(f)
        for row in reader:
            ev_dist.append({'N':float(row[0]), 'S':float(row[1]), 'V':float(row[2]), 'F':float(row[3])})

    #fs = ecg_signal[0]
    #min_A = ecg_signal[1]
    #max_A = ecg_signal[2]
    #n_bits = ecg_signal[3]
    #ecg_signal = ecg_signal[4:]   
    
    return ev_dist


# Read prob distribution from MLII
ev_dist_MLII = read_ev_dist('ev_dist_MLII_Shanon')
# Read prob distribution from RR
ev_dist_RR = read_ev_dist('ev_dist_RR_Shanon')
# for each instance

final_output = list()
for i in range(0, len(ev_dist_RR)):	
	# Make the mass asignation
	m1 = MassFunction(ev_dist_MLII[i])
	m2 = MassFunction(ev_dist_RR[i])

	# compute the combination with DS
	#m1_2_disj = m1.combine_disjunctive(m2)
	m1_2 = m1.combine_conjunctive(m2)

	# Pignistic to make a decision
	output = m1_2.pignistic()

	# Keep output
	output_array = [output['N'], output['S'], output['V'], output['F']]
	#print(output_array)
	max_val = max(output_array)
	decision = output_array.index(max_val)
	#print('Class: ' + str(decision) + ' val = ' +  str(max_val))

	final_output.append(decision)

# save the new output to a file
thefile = open('output_DS_Shanon_conjuntive.txt', 'w')

for item in final_output:
	thefile.write("%s\n" % item)
