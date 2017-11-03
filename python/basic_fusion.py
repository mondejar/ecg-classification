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

# Compute the basic rule from the list of probs 
# selected by rule index:
# 0 = product
# 1 = sum
# 2 = minimum
# and return the predictions

def basic_rules(probs_ensemble, rule_index):

    n_ensembles, n_instances, n_classes = probs_ensemble.shape

    predictions_rule = np.zeros(n_instances)

    # Product rule
    if rule_index == 0:
        probs_rule = np.ones([n_instances, n_classes])

        for p in range(n_instances):
            for e in range(n_ensembles):
                probs_rule[p] = probs_rule[p] * probs_ensemble[e,p]
            predictions_rule[p] = np.argmax(probs_rule[p])
    
    # Sum rule
    elif rule_index == 1:
        probs_rule = np.zeros([n_instances, n_classes])

        for p in range(n_instances):
            for e in range(n_ensembles):
                probs_rule[p] = probs_rule[p] + probs_ensemble[e,p]
            predictions_rule[p] = np.argmax(probs_rule[p])
    
    # Minimum rule
    elif rule_index == 2:
        probs_rule = np.ones([n_instances, n_classes])

        for p in range(n_instances):
            for e in range(n_ensembles):
                probs_rule[p] = np.minimum(probs_rule[p], probs_ensemble[e,p])
            predictions_rule[p] = np.argmax(probs_rule[p])

    # Maxmium rule
    elif rule_index == 3:
        probs_rule = np.zeros([n_instances, n_classes])

        for p in range(n_instances):
            for e in range(n_ensembles):
                probs_rule[p] = np.maximum(probs_rule[p], probs_ensemble[e,p])
            predictions_rule[p] = np.argmax(probs_rule[p])

    return predictions_rule


def main():
    print("Runing basic_fusion.py!")

    # Load gt labels
    eval_labels = np.loadtxt('/home/mondejar/Dropbox/ECG/code/ecg_classification/python/mit_db/DS2_labels.csv') 

    # Configuration
    results_path = '/home/mondejar/Dropbox/ECG/code/ecg_classification/python/results/'

    model_RR            = results_path + 'rm_bsln/' + 'maxRR/' + 'RR/' + 'norm_RR/'    + 'weighted/' + 'C_0.001' + '_prob_ovo.csv'
    model_wvl           = results_path + 'rm_bsln/' + 'maxRR/' + 'HOS/' + 'myMorph/'   + 'weighted/' + 'C_0.001' + '_prob_ovo.csv'
    model_HOS_myDesc    = results_path + 'rm_bsln/' + 'maxRR/' + 'wvlt/'               + 'weighted/' + 'C_0.001' + '_prob_ovo.csv'

    # Load Predictions!
    prob_ovo_RR         = np.loadtxt(model_RR)
    prob_ovo_wvl        = np.loadtxt(model_wvl)
    prob_ovo_HOS_myDesc = np.loadtxt(model_HOS_myDesc)
    
    # Compute sigmoid of the probs!
    #prob_ovo_RR_sig         = compute_sig_probs(prob_ovo_RR)
    #prob_ovo_wvl_sig        = compute_sig_probs(prob_ovo_wvl)
    #prob_ovo_HOS_myDesc_sig = compute_sig_probs(prob_ovo_HOS_myDesc)

    predict, prob_ovo_RR_sig            = voting_ovo(prob_ovo_RR)
    predict, prob_ovo_wvl_sig           = voting_ovo(prob_ovo_wvl)
    predict, prob_ovo_HOS_myDesc_sig    = voting_ovo(prob_ovo_HOS_myDesc)

    # TODO normalice to make sum up to 1?
    prob_ovo_RR_sig = prob_ovo_RR_sig / 6
    prob_ovo_wvl_sig = prob_ovo_wvl_sig / 6
    prob_ovo_HOS_myDesc_sig = prob_ovo_HOS_myDesc_sig / 6

    ##########################################################
    # Combine the predictions!
    ##########################################################
    probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig, prob_ovo_HOS_myDesc_sig))

    ###########################################
    # product rule!
    predictions_prob_rule = basic_rules(probs_ensemble, 0)
    perf_measures = compute_AAMI_performance_measures(predictions_prob_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/prod_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '.txt')


    ###########################################
    # Sum rule!
    predictions_sum_rule = basic_rules(probs_ensemble, 1)
    perf_measures = compute_AAMI_performance_measures(predictions_sum_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/sum_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '.txt')

    ############################################
    # min rule!
    predictions_min_rule = basic_rules(probs_ensemble, 2)
    perf_measures = compute_AAMI_performance_measures(predictions_min_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/min_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '.txt')

    ############################################
    # max rule!
    predictions_max_rule = basic_rules(probs_ensemble, 3)
    perf_measures = compute_AAMI_performance_measures(predictions_max_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/max_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '.txt')




    ######################################
    # Dempster Rule??
    # TODO how compute the confident factor for the evidence??
    
    #predictions_DS_rule = dempster_rule(probs_ensemble)
    #perf_measures = compute_AAMI_performance_measures(predictions_DS_rule.astype(int), eval_labels)
    #write_AAMI_results( perf_measures, results_path + 'fusion/DS_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '.txt')






if __name__ == '__main__':

    import sys

    main()
