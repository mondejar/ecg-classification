#!/usr/bin/env python

"""
train_SVM.py
    
VARPA, University of Coruna
Mondejar Guerra, Victor M.
23 Oct 2017
"""

from load_MITBIH import *
from evaluation_AAMI import *

import sklearn
from sklearn.externals import joblib
from sklearn.preprocessing import StandardScaler
from sklearn import svm

from imblearn.over_sampling import ADASYN, SMOTE, RandomOverSampler

import time

# Merge the OVO probs in final predictions.
# N = number of classes
# M = (N * (N-1))/ 2 OVO pairs
def voting_ovo(probs):
    predictions = np.zeros(len(probs))
    class_p = [0, 0, 0, 1, 1, 2]
    class_n = [1, 2, 3, 2, 3, 3]

    counter = np.zeros([len(probs), 4])

    for p in range(len(probs)):
        #for j in range(len(p)):
        #    prob_sig[j] = (1 / (1 + np.exp(-p[j]))))
        for i in range(len(probs[p])):
            counter[p, class_p[i]] = counter[p, class_p[i]] + (1 / (1 + np.exp(-probs[p,i])))
            counter[p, class_n[i]] = counter[p, class_n[i]] + (1 / (1 + np.exp(probs[p,i])))

        predictions[p] = np.argmax(counter[p])

    return predictions, counter

def create_svm_model_name(model_svm_path, winL, winR, do_preprocess, 
    maxRR, use_RR, norm_RR, compute_morph, use_weight_class, do_oversampling, delimiter):

    if do_preprocess:
        model_svm_path = model_svm_path + delimiter + 'rm_bsln'

    if maxRR:
        model_svm_path = model_svm_path + delimiter + 'maxRR'

    if use_RR:
        model_svm_path = model_svm_path + delimiter + 'RR'
    
    if norm_RR:
        model_svm_path = model_svm_path + delimiter + 'norm_RR'
    
    if 'wavelets' in compute_morph:
        model_svm_path = model_svm_path + delimiter + 'wvlt'

    if 'HOS' in compute_morph:
        model_svm_path = model_svm_path + delimiter + 'HOS'

    if 'myMorph' in compute_morph:
        model_svm_path = model_svm_path + delimiter + 'myMorph'

    if use_weight_class:
        model_svm_path = model_svm_path + delimiter + 'weighted'

    if do_oversampling:
        model_svm_path = model_svm_path + delimiter + 'SMOTE'

    return model_svm_path

def main(winL=90, winR=90, do_preprocess=True, use_weight_class=True, 
    maxRR=True, use_RR=True, norm_RR=True, compute_morph={''}):
    print("Runing train_SVM.py!")

    db_path = '/home/mondejar/dataset/ECG/mitdb/m_learning/scikit/'
    
    # Load train data 
    [tr_features, tr_labels] = load_mit_db('DS1', winL, winR, do_preprocess,
        maxRR, use_RR, norm_RR, compute_morph, db_path)
    # np.savetxt('mit_db/DS1_labels.csv', tr_labels.astype(int), '%.0f') 
    
    do_oversampling = False
    C_value = 0.001

    # TODO oversampling?
    if do_oversampling:
        # imbalanced-learn
        # (ratio='auto', random_state=None, k=None, k_neighbors=5, m=None, m_neighbors=10, out_step=0.5, kind='regular', svm_estimator=None, n_jobs=1)

        # ratio: 
        # (i) 'minority': resample the minority class; 
        # (ii) 'majority': resample the majority class, 
        # (iii) 'not minority': resample all classes apart of the minority class, 
        # (iv) 'all': resample all classes, and 
        # (v) 'auto': correspond to 'all' with for over-sampling methods and 'not minority' for under-sampling methods.
        #  The classes targeted will be over-sampled or under-sampled to achieve an equal number of sample with the majority or minority class.

        # kind =  'regular', 'borderline1', 'borderline2', 'svm'.

        # NOTE model for the SMOTE, the configuration must be the same later...
        svm_model = svm.SVC(C=C_value, kernel='rbf', degree=3, gamma='auto', 
            coef0=0.0, shrinking=True, probability=True, tol=0.001, 
            cache_size=200, class_weight='balanced', verbose=False, 
            max_iter=-1, decision_function_shape='ovo', random_state=None)
        # imblearn.over_sampling.SMOTE
        print("Computing oversampling features...")
        sm_svm = SMOTE(ratio='all', random_state=None, k_neighbors=5, m_neighbors=10, out_step=0.5, kind='svm', svm_estimator=svm_model, n_jobs=1)
        tr_features, tr_labels = sm_svm.fit_sample(tr_features, tr_labels)
        # TODO Write data oversampled!


    # Normalization of the input data
    # scaled: zero mean unit variance ( z-score )
    scaler = StandardScaler()
    scaler.fit(tr_features)
    tr_features_scaled = scaler.transform(tr_features)

    # NOTE for cross validation use: 
    # pipeline = Pipeline([('transformer', scalar), ('estimator', clf)])
    # instead of "StandardScaler()"

    ##############################
    # Train SVM model

    model_svm_path = db_path + 'svm_models/rbf'
    model_svm_path = create_svm_model_name(model_svm_path, winL, winR, 
        do_preprocess, maxRR, use_RR, norm_RR, compute_morph, use_weight_class, do_oversampling, '_')
    model_svm_path = model_svm_path + '_C_' +  str(C_value) + '.joblib.pkl'

    print("Training model on MIT-BIH DS1: " + model_svm_path + "...")

    if os.path.isfile(model_svm_path):
        # Load the trained model!
        svm_model = joblib.load(model_svm_path)

    else:
        class_weights = {}
        for c in range(4):
            class_weights.update({c:len(tr_labels) / float(np.count_nonzero(tr_labels == c))})

        #class_weight='balanced', 
        svm_model = svm.SVC(C=C_value, kernel='rbf', degree=3, gamma='auto', 
            coef0=0.0, shrinking=True, probability=True, tol=0.001, 
            cache_size=200, class_weight=class_weights, verbose=False, 
            max_iter=-1, decision_function_shape='ovo', random_state=None)
        
        # Let's Train!

        start = time.time()
        svm_model.fit(tr_features_scaled, tr_labels) 
        end = time.time()
        # TODO assert that the class_ID appears with the desired order, 
        # with the goal of ovo make the combinations properly
        print("Trained completed!\n\t" + model_svm_path + "\n \
            \tTime required: " + str(format(end - start, '.2f')) + " sec" )

        # Export model: save/write trained SVM model
        joblib.dump(svm_model, model_svm_path)

        # TODO Export StandardScaler()
    
    ##############################
    ## Test SVM model
    print("Testing model on MIT-BIH DS2: " + model_svm_path + "...")

    [eval_features, eval_labels] = load_mit_db('DS2', winL, winR, do_preprocess, 
        maxRR, use_RR, norm_RR, compute_morph, db_path)
    #np.savetxt('mit_db/DS2_labels.csv', eval_labels.astype(int), '%.0f') 

    # Normalization of the input data
    # scaled: zero mean unit variance ( z-score )
    eval_features_scaled = scaler.transform(eval_features)

    # Let's test new data!
    prob_ovo        = svm_model.decision_function(eval_features_scaled)
    predict_ovo, counter    = voting_ovo(prob_ovo)

    #predicts_log_proba  = svm_model.predict_log_proba(eval_features_scaled)
    #predicts_proba      = svm_model.predict_proba(eval_features_scaled)

    #predicts            = svm_model.predict(eval_features_scaled)

    print("Evaluation")
    perf_measures = compute_AAMI_performance_measures(predict_ovo, eval_labels)
    
    perf_measures_path = create_svm_model_name('/home/mondejar/Dropbox/ECG/code/ecg_classification/python/results', winL, winR, do_preprocess, 
        maxRR, use_RR, norm_RR, compute_morph, use_weight_class, do_oversampling, '/')
    # Write results and also predictions on DS2
    if not os.path.exists(perf_measures_path):
        os.makedirs(perf_measures_path)

    write_AAMI_results( perf_measures, perf_measures_path + '/C_' + str(C_value) + 
        '_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '.txt')
    
    # Array to .csv
    np.savetxt(perf_measures_path + '/C_' + str(C_value) +
        '_prob_ovo.csv', prob_ovo)

    np.savetxt(perf_measures_path + '/C_' + str(C_value) + 
        '_predict_ovo.csv', predict_ovo.astype(int), '%.0f') 

    print("Results writed at " + perf_measures_path + '/C_' + str(C_value))

    
if __name__ == '__main__':
    import sys

    winL = sys.argv[1]
    winR = sys.argv[2]
    do_preprocess = sys.argv[3]
    use_weight_class = sys.argv[4]
    maxRR = sys.argv[5]
    use_RR = sys.argv[6]
    norm_RR = sys.argv[7]
    
    compute_morph = {''} # 'wavelets', 'HOS', 'myMorph'
    for s in sys.argv[8:]:
        compute_morph.add(s)
    
    main(winL, winR, do_preprocess, use_weight_class, maxRR, use_RR, norm_RR, compute_morph)
