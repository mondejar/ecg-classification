#!/usr/bin/env python

"""
run_SVM.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
15 Dec 2017
"""
from train_SVM import *


# Run the cross val for all the modules (RR, Wavlets, HOS)

winL = 90
winR = 90
do_preprocess = True
use_weight_class = True
multi_mode = 'ovo'
#voting_strategy = 'ovo_voting'  # 'ovo_voting_exp', 'ovo_voting_both'

oversamp_methods = {''} #,, 'SMOTE', 'SMOTEENN', 'SMOTETomek', 'ADASYN'
feature_selections = {''}#, 'SFS', 'CFS', 'RS'

do_cross_val = 'pat_cv' # 'beat_cv' 

feature_selection = ''
maxRR = True

pca_k = 0

for oversamp_method in oversamp_methods:

    print("run_full_crossval.py: SVM ((RR) train with oversamp " + oversamp_method)

    use_RR = False
    norm_RR = False
    compute_morph = {'wvlt'} # 'wvlt', 'HOS', 'myMorph'
    main(multi_mode, 90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph, oversamp_method, pca_k, feature_selection, do_cross_val)

    """
    ####################################
    # No feature selecion!
    
    # Raw + Feature selection!
    use_RR = False
    norm_RR = False
    compute_morph = {'raw'} # 'wvlt', 'HOS', 'myMorph'
    
    feature_selection = ''
    main(multi_mode, 90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph, oversamp_method, pca_k, feature_selection, do_cross_val)
    """


    """
    print("run_full_crossval.py: SVM ((RR) train with oversamp " + oversamp_method)
    # RR
    use_RR = True
    norm_RR = True
    compute_morph = {''} # 'wvlt', 'HOS', 'myMorph', 'raw'
    main(multi_mode, 90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph, oversamp_method, pca_k, feature_selection, do_cross_val)

 
    print("run_full_crossval.py: SVM (Wavelet) train with oversamp " + oversamp_method)

    # Wavelet
    use_RR = False
    norm_RR = False
    compute_morph = {'wvlt'} # 'wvlt', 'HOS', 'myMorph', 'raw'
    main(multi_mode, 90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph, oversamp_method, pca_k, feature_selection, do_cross_val)



    
    # HOS my morph
    print("run_full_crossval.py: SVM (HOS) train with oversamp " + oversamp_method)

    use_RR = False
    norm_RR = False
    compute_morph = {'HOS'} # 'wvlt', 'HOS', 'myMorph'
    main(multi_mode, 90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph, oversamp_method, pca_k, feature_selection, do_cross_val)
    
    # HOS my morph
    compute_morph = {'HOS', 'myMorph'} # 'wvlt', 'HOS', 'myMorph'
    main(multi_mode, 90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph, oversamp_method, pca_k, feature_selection, do_cross_val)


    print("run_full_crossval.py: SVM (RR + Wavelet + HOS) train with oversamp " + oversamp_method)


    """

    """
    # RR + Wavelet + HOS_my_morph
    use_RR = True
    norm_RR = True
    compute_morph = {'wvlt', 'HOS', 'myMorph'} # 'wvlt', 'HOS', 'myMorph', 'raw'
    main(multi_mode, 90, 90, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph, oversamp_method, pca_k, feature_selection, do_cross_val)
    """