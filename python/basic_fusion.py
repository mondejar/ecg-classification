#!/usr/bin/env python

"""
basic_fusion.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
30 Oct 2017
"""

from train_SVM import *


def compute_sig_probs(probs):
    probs_sig = np.zeros(probs.shape)
    for i in range(len(probs)):
        for j in range(len(probs[i])):
            probs_sig[i,j] = (1 / (1 + np.exp(-probs[i,j])))
    
    return probs_sig

def main():
    print("Runing basic_fusion.py!")

    # Load gt labels
    eval_labels = np.loadtxt('/home/mondejar/Dropbox/ECG/code/ecg_classification/python/mit_db/DS2_labels.csv') 

    # Configuration
    results_path = '/home/mondejar/Dropbox/ECG/code/ecg_classification/python/results/'

    model_RR            = results_path + 'rm_bsln/' + 'RR/' + 'norm_RR/'    + 'weighted/' + 'C_0.001' + '_prob_ovo.csv'
    model_wvl           = results_path + 'rm_bsln/' + 'HOS/' + 'myMorph/'   + 'weighted/' + 'C_0.001' + '_prob_ovo.csv'
    model_HOS_myDesc    = results_path + 'rm_bsln/' + 'wvlt/'               + 'weighted/' + 'C_0.001' + '_prob_ovo.csv'

    # Load Predictions!
    prob_ovo_RR = np.loadtxt(model_RR)
    prob_ovo_wvl = np.loadtxt(model_wvl)
    prob_ovo_HOS_myDesc = np.loadtxt(model_HOS_myDesc)
    
    # Compute sigmoid of the probs!
    #prob_ovo_RR_sig         = compute_sig_probs(prob_ovo_RR)
    #prob_ovo_wvl_sig        = compute_sig_probs(prob_ovo_wvl)
    #prob_ovo_HOS_myDesc_sig = compute_sig_probs(prob_ovo_HOS_myDesc)

    predict, prob_ovo_RR_sig = voting_ovo(prob_ovo_RR)
    predict, prob_ovo_wvl_sig = voting_ovo(prob_ovo_wvl)
    predict, prob_ovo_HOS_myDesc_sig = voting_ovo(prob_ovo_HOS_myDesc)

    # TODO normalice to make sum up to 1?

    # Combine the predictions!
    print("Product Rule")

    # product rule!
    probs_prod_rule = np.zeros(prob_ovo_RR_sig.shape)
    predictions_prod_rule = np.zeros(len(prob_ovo_RR_sig))

    for p in range(len(prob_ovo_RR_sig)):
        probs_prod_rule[p,:] = prob_ovo_RR_sig[p] * prob_ovo_wvl_sig[p] * prob_ovo_HOS_myDesc_sig[p]
        predictions_prod_rule[p] = np.argmax(probs_prod_rule[p])

    predictions_prod_rule = predictions_prod_rule.astype(int)
    # mayority 
    # Evaluate!
    print("Evaluation")
    perf_measures = compute_AAMI_performance_measures(predictions_prod_rule, eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/prod_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '.txt')


if __name__ == '__main__':

    import sys

    main()
