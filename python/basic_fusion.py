#!/usr/bin/env python

"""
basic_fusion.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
30 Oct 2017
"""

from train_SVM import *

# Compute the basic rule from the list of probs 
# selected by rule index:
# 0 = product
# 1 = sum
# 2 = minimum
# 3 = minimum
# 4 = majority
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

    # Maximum rule
    elif rule_index == 3:
        probs_rule = np.zeros([n_instances, n_classes])

        for p in range(n_instances):
            for e in range(n_ensembles):
                probs_rule[p] = np.maximum(probs_rule[p], probs_ensemble[e,p])
            predictions_rule[p] = np.argmax(probs_rule[p])
    
    # Majority rule
    elif rule_index == 4:
        rank_rule = np.zeros([n_instances, n_classes])
        # Just simply adds the position of the ranking 
        for p in range(n_instances):

            for e in range(n_ensembles):
                rank = np.argsort(probs_ensemble[e,p])
                for j in range(n_classes):
                    rank_rule[p,rank[j]] = rank_rule[p,rank[j]] + j
            predictions_rule[p] = np.argmax(rank_rule[p])

    return predictions_rule


def main():
    DS = 'DS2'
    print("Runing basic_fusion.py!" + DS)

    oversamp = '' #'', 'SMOTEENN/', 'SMOTE/', 'SMOTETomek/', 'ADASYN/'
    # Load gt labelso
    eval_labels = np.loadtxt('/home/mondejar/Dropbox/ECG/code/ecg_classification/python/mit_db/' + DS + '_labels.csv') 

    # Configuration
    results_path = '/home/mondejar/Dropbox/ECG/code/ecg_classification/python/results/ovo/MLII/'

    if DS == 'DS2':     
        model_RR            = results_path + oversamp + 'rm_bsln/' + 'maxRR/' + 'RR/' + 'norm_RR/'   + 'weighted/' + 'C_0.001' + '_decision_ovo.csv'
        model_wvl           = results_path + oversamp + 'rm_bsln/' + 'maxRR/' + 'wvlt/'              + 'weighted/' + 'C_0.001' + '_decision_ovo.csv'       
        model_LBP           = results_path + oversamp + 'rm_bsln/' + 'maxRR/' + 'lbp/'               + 'weighted/' + 'C_0.001' + '_decision_ovo.csv' 
        model_HOS           = results_path + oversamp + 'rm_bsln/' + 'maxRR/' + 'HOS/'               + 'weighted/' + 'C_0.001' + '_decision_ovo.csv'
        model_myDesc        = results_path + oversamp + 'rm_bsln/' + 'maxRR/' + 'myMorph/'           + 'weighted/' + 'C_0.001' + '_decision_ovo.csv'

    # Load Predictions!

    prob_ovo_RR         = np.loadtxt(model_RR)
    prob_ovo_wvl        = np.loadtxt(model_wvl)
    prob_ovo_LBP        = np.loadtxt(model_LBP)
    prob_ovo_HOS        = np.loadtxt(model_HOS)
    prob_ovo_MyDescp    = np.loadtxt(model_myDesc)


    prob_ovo_HBF    = np.loadtxt(model_HBF)
   
    predict, prob_ovo_RR_sig      = ovo_voting_exp(prob_ovo_RR, 4) #voting_ovo_w(prob_ovo_RR)           #voting_ovo_raw(prob_ovo_RR)
    predict, prob_ovo_wvl_sig     = ovo_voting_exp(prob_ovo_wvl, 4) #voting_ovo_w(prob_ovo_wvl)          #voting_ovo_raw(prob_ovo_wvl)
    predict, prob_ovo_LBP_sig     = ovo_voting_exp(prob_ovo_LBP, 4) #voting_ovo_w(prob_ovo_HOS_myDesc)   #voting_ovo_raw(prob_ovo_HOS_myDesc)
    predict, prob_ovo_HOS_sig     = ovo_voting_exp(prob_ovo_HOS, 4) #voting_ovo_w(prob_ovo_HOS_myDesc)   #voting_ovo_raw(prob_ovo_HOS_myDesc)
    predict, prob_ovo_MyDescp_sig = ovo_voting_exp(prob_ovo_MyDescp, 4) #voting_ovo_w(prob_ovo_HOS_myDesc)   #voting_ovo_raw(prob_ovo_HOS_myDesc)

    predict, prob_ovo_HBF_sig = ovo_voting_exp(prob_ovo_HBF, 4) 

    ##########################################################
    # Combine the predictions!
    ##########################################################
    # 2
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_HOS_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_LBP_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_MyDescp_sig))

    #probs_ensemble = np.stack((prob_ovo_wvl_sig, prob_ovo_HOS_sig))
    #probs_ensemble = np.stack((prob_ovo_wvl_sig, prob_ovo_LBP_sig))
    #probs_ensemble = np.stack((prob_ovo_wvl_sig, prob_ovo_MyDescp_sig))
    
    #probs_ensemble = np.stack((prob_ovo_HOS_sig, prob_ovo_LBP_sig))
    #probs_ensemble = np.stack((prob_ovo_HOS_sig, prob_ovo_MyDescp_sig))

    #probs_ensemble = np.stack((prob_ovo_LBP_sig, prob_ovo_MyDescp_sig))

    # 3
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig, prob_ovo_HOS_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig, prob_ovo_LBP_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig, prob_ovo_MyDescp_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_HOS_sig, prob_ovo_LBP_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_HOS_sig, prob_ovo_MyDescp_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_LBP_sig, prob_ovo_MyDescp_sig))

    #probs_ensemble = np.stack((prob_ovo_wvl_sig, prob_ovo_HOS_sig, prob_ovo_LBP_sig))
    #probs_ensemble = np.stack((prob_ovo_wvl_sig, prob_ovo_HOS_sig, prob_ovo_MyDescp_sig))
    #probs_ensemble = np.stack((prob_ovo_wvl_sig, prob_ovo_LBP_sig, prob_ovo_MyDescp_sig))
    #probs_ensemble = np.stack((prob_ovo_HOS_sig, prob_ovo_LBP_sig, prob_ovo_MyDescp_sig))

    # 4
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig, prob_ovo_HOS_sig, prob_ovo_LBP_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig, prob_ovo_HOS_sig, prob_ovo_MyDescp_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig, prob_ovo_LBP_sig, prob_ovo_MyDescp_sig))
    #probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_HOS_sig, prob_ovo_LBP_sig, prob_ovo_MyDescp_sig))
    #probs_ensemble = np.stack((prob_ovo_wvl_sig, prob_ovo_HOS_sig, prob_ovo_LBP_sig, prob_ovo_MyDescp_sig))

    # 5
    probs_ensemble = np.stack((prob_ovo_RR_sig, prob_ovo_wvl_sig, prob_ovo_HOS_sig, prob_ovo_LBP_sig, prob_ovo_MyDescp_sig))

    n_ensembles, n_instances, n_classes = probs_ensemble.shape
    
    ###########################################
    # product rule!
    predictions_prob_rule = basic_rules(probs_ensemble, 0)
    perf_measures = compute_AAMI_performance_measures(predictions_prob_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/prod_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_' + DS +  '.txt')
        
    ###########################################
    # Sum rule!
    """
    predictions_sum_rule = basic_rules(probs_ensemble, 1)
    perf_measures = compute_AAMI_performance_measures(predictions_sum_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/sum_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_' + DS +  '.txt')
    """
    # min rule!
    """
    predictions_min_rule = basic_rules(probs_ensemble, 2)
    perf_measures = compute_AAMI_performance_measures(predictions_min_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/min_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_' + DS + '.txt')
    """
    
    # max rule!
    """
    predictions_max_rule = basic_rules(probs_ensemble, 3)
    perf_measures = compute_AAMI_performance_measures(predictions_max_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/max_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_' + DS + '.txt')
    """

    # Mayority rule / Ranking
    """
    predictions_rank_rule = basic_rules(probs_ensemble, 4)
    perf_measures = compute_AAMI_performance_measures(predictions_rank_rule.astype(int), eval_labels)
    write_AAMI_results( perf_measures, results_path + 'fusion/rank_rule_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_' + DS + '.txt')
    """

if __name__ == '__main__':

    import sys

    main()
