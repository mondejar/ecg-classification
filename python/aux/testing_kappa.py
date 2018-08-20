
from sklearn import metrics
import numpy as np

def compute_cohen_kappa(confusion_matrix):
    prob_expectedA = np.empty(len(confusion_matrix))
    prob_expectedB = np.empty(len(confusion_matrix))
    prob_observed = 0
    
    mean_acc = np.empty(len(confusion_matrix))
    for n in range(0, len(confusion_matrix)):
        prob_expectedA[n] = sum(confusion_matrix[n,:]) / sum(sum(confusion_matrix))
        prob_expectedB[n] = sum(confusion_matrix[:,n]) / sum(sum(confusion_matrix))
    
        prob_observed = prob_observed + confusion_matrix[n][n]
        mean_acc[n] = confusion_matrix[n][n]  / sum(confusion_matrix[:,n])

    prob_expected = np.dot(prob_expectedA, prob_expectedB)
    prob_observed = prob_observed / sum(sum(confusion_matrix))  

    kappa = (prob_observed - prob_expected) / (1 - prob_expected)

    print("Kappa: " + str(kappa) + " Po: " + str(prob_observed) + " Pe: " + str(prob_expected) + "\nPe_a: " + str(prob_expectedA) + "\nPe_b: " + str(prob_expectedB) )
    print("Mean acc = " + str(np.sum(mean_acc) / 4.) + " m_acc = " + str(mean_acc))

    print("Kappa / Acc")
    print(str(kappa / prob_observed))


    print("Linear relationship y = mx + w:\t prob_observed * (1./0.75)  + (0.25/0.75)")
    print( str( (prob_observed * (1./0.75) +  (-0.25/0.75) )))

    return kappa, prob_observed, prob_expected


conf_mat = np.array([[33881, 2531, 2385, 5236], [263, 1036, 725, 26], [63, 350, 2584, 223], [43, 2, 4, 339]])
print(conf_mat)
conf_mat = conf_mat.astype(float)
compute_cohen_kappa(conf_mat)

print("\n\n")
conf_mat = np.array([[25171, 11907, 1125, 5830], [340, 1474, 39, 197], [212, 486, 2369, 153], [31, 5, 55, 297]])
print(conf_mat)
conf_mat = conf_mat.astype(float)
compute_cohen_kappa(conf_mat)

conf_mat = np.array([[50, 0,0 ,0 ], [0, 50, 0, 0], [0, 0, 50, 0], [0, 0, 0, 50]])
print(conf_mat)
conf_mat = conf_mat.astype(float)
compute_cohen_kappa(conf_mat)

print("\n\n")
conf_mat = np.array([[40, 0, 5 ,5 ], [3, 35, 2, 10], [2, 3, 45, 0], [0, 0, 0, 50]])
print(conf_mat)
conf_mat = conf_mat.astype(float)
compute_cohen_kappa(conf_mat)


print("\n\n")
conf_mat = np.array([[20, 0, 15 ,15 ], [5, 33, 5, 7], [3, 3, 43,1], [1, 6, 3, 40]])
print(conf_mat)
conf_mat = conf_mat.astype(float)
compute_cohen_kappa(conf_mat)


print("\n\n")
conf_mat = np.array([[10, 10, 15 ,15 ], [5, 33, 5, 7], [6, 0, 3,41], [1, 6, 3, 40]])
print(conf_mat)
conf_mat = conf_mat.astype(float)
compute_cohen_kappa(conf_mat)
