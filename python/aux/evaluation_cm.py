#!/usr/bin/env python

"""
evaluation_cnn.py
    
Description:
    Auxiliar file to compute performance measures given the confussion matrix

VARPA, University of Coruna
Mondejar Guerra, Victor M.
31 Jan 2018
"""

from sklearn import metrics
import numpy as np

class performance_measures:
    def __init__(self, n):
        self.n_classes          = n
        self.confusion_matrix   = np.empty([])
        self.Recall             = np.empty(n)
        self.Precision          = np.empty(n)
        self.Specificity        = np.empty(n)
        self.Acc                = np.empty(n)
        self.F_measure          = np.empty(n)

        self.gmean_se          = 0.0
        self.gmean_p      #!/usr/bin/env python

"""
train_SVM.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
26 Oct 2017
"""

from sklearn import metrics
import numpy as np

class performance_measures:
    def __init__(self, n):
        self.n_classes          = n
        self.confusion_matrix   = np.empty([])
        self.Recall             = np.empty(n)
        self.Precision          = np.empty(n)
        self.Specificity        = np.empty(n)
        self.Acc                = np.empty(n)
        self.F_measure          = np.empty(n)

        self.gmean_se          = 0.0
        self.gmean_p           = 0.0

        self.Overall_Acc        = 0.0
        self.kappa              = 0.0
        self.Ij                 = 0.0
        self.Ijk                = 0.0

        self.Overall_Acc        = 0.0
        self.kappa              = 0.0
        self.Ij                 = 0.0
        self.Ijk                = 0.0


# Compute Cohen' kappa from a confussion matrix
# Kappa value:
#    < 0.20  Poor
# 0.21-0.40  Fair
# 0.41-0.60  Moderate
# 0.61-0.80  Good
# 0.81-1.00  Very good
def compute_cohen_kappa(confusion_matrix):
    prob_expectedA = np.empty(len(confusion_matrix))
    prob_expectedB = np.empty(len(confusion_matrix))
    prob_observed = 0
    
    for n in range(0, len(confusion_matrix)):
        prob_expectedA[n] = sum(confusion_matrix[n,:]) / sum(sum(confusion_matrix))
        prob_expectedB[n] = sum(confusion_matrix[:,n]) / sum(sum(confusion_matrix))
    
        prob_observed = prob_observed + confusion_matrix[n][n]

    prob_expected = np.dot(prob_expectedA, prob_expectedB)
    prob_observed = prob_observed / sum(sum(confusion_matrix))  

    kappa = (prob_observed - prob_expected) / (1 - prob_expected)

    return kappa, prob_observed, prob_expected



# Export to filename.txt file the performance measure score
def write_AAMI_results(performance_measures, filename):

    f = open(filename, "w") 

    f.write("Ijk: " + str(format(performance_measures.Ijk, '.4f')) + "\n")
    f.write("Ij: " + str(format(performance_measures.Ij, '.4f'))+ "\n")
    f.write("Cohen's Kappa: " + str(format(performance_measures.kappa, '.4f'))+ "\n\n")

    # Conf matrix
    f.write("Confusion Matrix:"+ "\n\n")
    f.write("\n".join(str(elem) for elem in performance_measures.confusion_matrix.astype(int))+ "\n\n")

    f.write("Overall ACC: " + str(format(performance_measures.Overall_Acc, '.4f'))+ "\n\n")

    f.write("mean Acc: " + str(format(np.average(performance_measures.Acc[:]), '.4f'))+ "\n")
    f.write("mean Recall: " + str(format(np.average(performance_measures.Recall[:]), '.4f'))+ "\n")
    f.write("mean Precision: " + str(format(np.average(performance_measures.Precision[:]), '.4f'))+ "\n")
  

    f.write("N:"+ "\n\n")
    f.write("Sens: " + str(format(performance_measures.Recall[0], '.4f'))+ "\n")
    f.write("Prec: " + str(format(performance_measures.Precision[0], '.4f'))+ "\n")
    f.write("Acc: " + str(format(performance_measures.Acc[0], '.4f'))+ "\n")

    f.write("SVEB:"+ "\n\n")
    f.write("Sens: " + str(format(performance_measures.Recall[1], '.4f'))+ "\n")
    f.write("Prec: " + str(format(performance_measures.Precision[1], '.4f'))+ "\n")
    f.write("Acc: " + str(format(performance_measures.Acc[1], '.4f'))+ "\n")

    f.write("VEB:"+ "\n\n")
    f.write("Sens: " + str(format(performance_measures.Recall[2], '.4f'))+ "\n")
    f.write("Prec: " + str(format(performance_measures.Precision[2], '.4f'))+ "\n")
    f.write("Acc: " + str(format(performance_measures.Acc[2], '.4f'))+ "\n")

    f.write("F:"+ "\n\n")
    f.write("Sens: " + str(format(performance_measures.Recall[3], '.4f'))+ "\n")
    f.write("Prec: " + str(format(performance_measures.Precision[3], '.4f'))+ "\n")
    f.write("Acc: " + str(format(performance_measures.Acc[3], '.4f'))+ "\n")


    f.close()





def compute_AAMI_performance_measures(conf_mat):
    n_classes = 4 #5
    pf_ms = performance_measures(n_classes)

    pf_ms.confusion_matrix = conf_mat

    # Overall Acc
    pf_ms.Overall_Acc = 0.0

    # AAMI: Sens, Spec, Acc 
    # N: 0, S: 1, V: 2, F: 3                        # (Q: 4) not used
    for i in range(0, n_classes):
        TP = conf_mat[i,i]
        FP = sum(conf_mat[:,i]) - conf_mat[i,i]
        TN = sum(sum(conf_mat)) - sum(conf_mat[i,:]) - sum(conf_mat[:,i]) + conf_mat[i,i]
        FN = sum(conf_mat[i,:]) - conf_mat[i,i]

        if i == 2: # V 
            # Exceptions for AAMI recomendations:
            # 1 do not reward or penalize a classifier for the classification of (F) as (V)
            FP = FP - conf_mat[i][3]
        
        pf_ms.Recall[i]       = TP / (TP + FN)
        pf_ms.Precision[i]    = TP / (TP + FP)
        pf_ms.Specificity[i]  = TN / (TN + FP); # 1-FPR
        pf_ms.Acc[i]          = (TP + TN) / (TP + TN + FP + FN)

        if TP == 0:
            pf_ms.F_measure[i] = 0.0
        else:
            pf_ms.F_measure[i] = 2 * (pf_ms.Precision[i] * pf_ms.Recall[i] )/ (pf_ms.Precision[i] + pf_ms.Recall[i])

    # Compute Cohen'    # TODO If conf_mat no llega a clases 4 por gt_labels o predictions...
    # hacer algo para que no falle el codigo...
    # NOTE: added labels=[0,1,2,3])...

    # Confussion matrixs Kappa
    pf_ms.kappa, prob_obsv, prob_expect = compute_cohen_kappa(conf_mat)

    # Compute Index-j   recall_S + recall_V + precision_S + precision_V
    pf_ms.Ij = pf_ms.Recall[1] + pf_ms.Recall[2] + pf_ms.Precision[1] + pf_ms.Precision[2]

    # Compute Index-jk
    w1 = 0.5
    w2 = 0.125
    pf_ms.Ijk = w1 * pf_ms.kappa + w2 * pf_ms.Ij

    return pf_ms


results_path = '/home/mondejar/Dropbox/ECG/code/ecg_classification/python/results/ovo/MLII/'

# Our single SVM
#conf_mat = np.array([[39446, 2404, 340, 1843], [443, 1374, 186, 47], [28, 162, 3005, 25], [240, 2, 35, 111]])

# Our features individually
## RR C_0.001_IJK_0.44
conf_mat = np.array([[33881, 2531, 2385, 5236], [263, 1036, 725, 26], [63, 350, 2584, 223], [43, 2, 4, 339]])
conf_mat = conf_mat.astype(float)
perf_measures = compute_AAMI_performance_measures(conf_mat)
write_AAMI_results( perf_measures, results_path + 'RR_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_DS2.txt')


## HOS
conf_mat = np.array([[25171, 11907, 1125, 5830], [340, 1474, 39, 197], [212, 486, 2369, 153], [31, 5, 55, 297]])
conf_mat = conf_mat.astype(float)
perf_measures = compute_AAMI_performance_measures(conf_mat)
write_AAMI_results( perf_measures, results_path + 'HOS_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_DS2.txt')

## W C_0.001 IJK 0.38
conf_mat = np.array([[37752, 2484, 3722, 75], [1755, 217, 76, 2], [98, 27, 3087, 8], [12, 1, 370, 5]])
conf_mat = conf_mat.astype(float)
perf_measures = compute_AAMI_performance_measures(conf_mat)
write_AAMI_results( perf_measures, results_path + 'W_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_DS2.txt')


## Our Morph C_0.001 IJK 0.35
conf_mat = np.array([[20601, 11239, 1817, 10376], [327, 1450, 16, 257], [302, 165, 2483, 270], [277, 78, 21, 12]])
conf_mat = conf_mat.astype(float)
perf_measures = compute_AAMI_performance_measures(conf_mat)
write_AAMI_results( perf_measures, results_path + 'Our_morph_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_DS2.txt')

## LBP C_0.001 IJK 0.17
conf_mat = np.array([[32771, 1211, 4256, 5795], [1922, 11, 51, 66], [847, 91, 1688, 594], [27, 0, 360, 1]])
conf_mat = conf_mat.astype(float)
perf_measures = compute_AAMI_performance_measures(conf_mat)
write_AAMI_results( perf_measures, results_path + 'LBP_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_DS2.txt')

# Chazal et al
#conf_mat = np.array([[38444, 1904, 303, 3509], [173, 1395, 252, 16], [117, 321, 2504, 176], [33, 1, 7, 347]])

#conf_mat = conf_mat.astype(float)
#perf_measures = compute_AAMI_performance_measures(conf_mat)
#write_AAMI_results( perf_measures, results_path + 'Chazal_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '_DS2.txt')
