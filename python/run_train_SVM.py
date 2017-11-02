#!/usr/bin/env python

"""
run_SVM.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
27 Oct 2017
"""

from train_SVM import *

# Call different configurations for train_SVM.py 

####################################################################################
winL = 90
winR = 90
do_preprocess = True
use_weight_class = True
maxRR = False
use_RR = True
norm_RR = True
compute_morph = {''} # 'wavelets', 'HOS', 'myMorph'

#main(90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph)
#################################################################################### 

maxRR = True

#RR maxRR
#main(90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph)


use_RR = False
norm_RR = False
compute_morph = {'wavelets'} # 'wavelets', 'HOS', 'myMorph'

main(90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph)
####################################################################################

compute_morph = {'HOS', 'myMorph'} # 'wavelets', 'HOS', 'myMorph'
main(90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph)


