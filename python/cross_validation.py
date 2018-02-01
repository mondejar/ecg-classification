#!/usr/bin/env python

"""
train_SVM.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
15 Dec 2017
"""
from evaluation_AAMI import *
from sklearn import svm
from aggregation_voting_strategies import *


# Eval the SVM model and export the results
def eval_crossval_fold(svm_model, features, labels, multi_mode, voting_strategy):
    if multi_mode == 'ovo':
        decision_ovo        = svm_model.decision_function(features)

        if voting_strategy == 'ovo_voting':
            predict_ovo, counter    = ovo_voting(decision_ovo, 4)

        elif voting_strategy == 'ovo_voting_both':
            predict_ovo, counter    = ovo_voting_both(decision_ovo, 4)

        elif voting_strategy == 'ovo_voting_exp':
            predict_ovo, counter    = ovo_voting_exp(decision_ovo, 4)

        # svm_model.predict_log_proba  svm_model.predict_proba   svm_model.predict ...
        perf_measures = compute_AAMI_performance_measures(predict_ovo, labels)

    return perf_measures


def run_cross_val(features, labels, patient_num_beats, division_mode, k):
    print("Runing Cross validation...")

    # C_values
    # gamma_values
    C_values = [0.0001, 0.001, 0.01, 0.1, 1, 10, 50, 100, 200, 1000]
    #ijk_scores = np.zeros(len(C_values))
    cv_scores  = np.zeros(len(C_values))
    index_cv = 0
    n_classes = 4

    for c_svm in C_values:
        # for g in g_values...

        features_k_fold = [np.array([]) for i in range(k)]
        label_k_fold = [np.array([]) for i in range(k)]

        ################
        # PREPARE DATA
        ################
        if division_mode == 'pat_cv':
            k = 22
            base = 0
            for kk in range(k):
                features_k_fold[kk] = features[base:base+patient_num_beats[kk]]
                label_k_fold[kk]    = labels[base:base+patient_num_beats[kk]]
                base = patient_num_beats[kk] + 1

        # NOTE: 22 k-folds will be very computational cost
        # NOTE: division by patient and oversampling couldnt by used!!!!
        if division_mode == 'beat_cv':
            
            # NOTE: class sklearn.model_selection.StratifiedKFold(n_splits=3, shuffle=False, random_state=None)[source]
            # Stratified K-Folds cross-validator
            # Provides train/test indices to split data in train/test sets.
            # Thirun_cross_vals cross-validation object is a variation of KFold that returns stratified folds. 
            # The folds are made by preserving the percentage of samples for each class!!

            # Sort features and labels by class ID
            features_by_class = {}

            for c in range(n_classes):
                features_by_class[c] = features[labels == c]
                
                # Then split each instances group in k-folds
                # generate the k-folds with the same class ID proportions in each one
                instances_class = len(features_by_class[c])
                increment = instances_class / k
                base = 0
                for kk in range(k):
                    features_k_fold[kk] = np.vstack((features_k_fold[kk], features_by_class[c][base:base + increment]))  if features_k_fold[kk].size else features_by_class[c][base:base+increment]
                    label_k_fold[kk] = np.hstack((label_k_fold[kk], np.zeros(increment) + c))  if label_k_fold[kk].size else np.zeros(increment) + c
                    base = increment + 1
            


        ################
        # RUN CROSS VAL
        ################
        for kk in range(k):
            # Rotate each iteration one fold for test and the rest for training

            # for each k-fold select the train and validation data
            val_features = features_k_fold[kk]
            val_labels = label_k_fold[kk]
            tr_features = np.array([])
            tr_labels =np.array([])
            for kkk in range(k):
                if kkk != kk:
                    tr_features = np.vstack((tr_features, features_k_fold[kkk])) if tr_features.size else features_k_fold[kkk]# select k fold
                    tr_labels =  np.append(tr_labels, label_k_fold[kkk])

            # pipeline = Pipeline([('transformer', scalar), ('estimator', clf)])
            # instead of "StandardScaler()"

            ####################################################33
            # Train
            multi_mode = 'ovo'
            class_weights = {}
            for c in range(n_classes):
                class_weights.update({c:len(tr_labels) / float(np.count_nonzero(tr_labels == c))})
            #class_weight='balanced', 
            svm_model = svm.SVC(C=c_svm, kernel='rbf', degree=3, gamma='auto', 
                coef0=0.0, shrinking=True, probability=False, tol=0.001, 
                cache_size=200, class_weight=class_weights, verbose=False, 
                max_iter=-1, decision_function_shape=multi_mode, random_state=None)
            
            # Let's Train!
            svm_model.fit(tr_features, tr_labels) 
        
            #########################################################################
            # 4) Test SVM model
            # ovo_voting:
            # Simply add 1 to the win class
            perf_measures = eval_crossval_fold(svm_model, val_features, val_labels, multi_mode, 'ovo_voting_exp')
            
            # TODO evaluar con el propio Ijk?? de esta manera obtendremos el SVM entrenando en maximizar esa medida...
            #ijk_scores[index_cv] += perf_measures.Ijk

            cv_scores[index_cv] += np.average(perf_measures.F_measure)

            # TODO g-mean?
            # Zhang et al computes the g-mean. But they computed the g-mean value for each SVM model of the 1 vs 1. NvsS, NvsV, ..., SvsV....

            # NOTE we could use the F-measure average from each class??

            print("C value (" + str(c_svm) + " Cross val k " + str(kk) +  "/" + str(k) + "  AVG(F-measure) = " + str( cv_scores[index_cv] / float(kk+1))) 
        # range k
        # beat division
        
        cv_scores[index_cv] /= float(k)# Average this result with the rest of the k-folds
        # NOTE: what measure maximize in the cross val???? 

        index_cv += 1

    # c_values
    
    return cv_scores, C_values
