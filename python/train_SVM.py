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

def create_svm_model_name(model_svm_path, winL, winR, do_preprocess, use_RR, norm_RR, compute_morph, use_weight_class, delimiter):

    if do_preprocess:
        model_svm_path = model_svm_path + delimiter + 'rm_bsln'
    
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

    return model_svm_path

def main(args):
    print("Runing train_SVM.py!")

    db_path = '/home/mondejar/dataset/ECG/mitdb/m_learning/'

    # Define configuration
    winL = 90
    winR = 90
    do_preprocess = True
    use_weight_class = True
    maxRR = True
    use_RR = True
    norm_RR = True
    compute_morph = {} # 'wavelets', 'HOS'
    
    # Load train data 
    [tr_features, tr_labels] = load_mit_db('DS1', winL, winR, do_preprocess, maxRR, use_RR, norm_RR, compute_morph, db_path)
    # np.savetxt('mit_db/DS1_labels.csv', tr_labels.astype(int), '%.0f') 
    # Do oversampling?

    # Normalization of the input data
    # scaled: zero mean unit variance ( z-score )
    scaler = StandardScaler()
    scaler.fit(tr_features)
    tr_features_scaled = scaler.transform(tr_features)
    # NOTE for cross validation use: 
    # pipeline = Pipeline([('transformer', scalar), ('estimator', clf)]) instead of "StandardScaler()"

    ##############################
    # Train SVM model
    C_value = 1.0

    model_svm_path = db_path + 'svm_models_py/rbf'
    model_svm_path = create_svm_model_name(model_svm_path, winL, winR, do_preprocess, use_RR, norm_RR, compute_morph, use_weight_class, '_')
    model_svm_path = model_svm_path + '_C_' +  str(C_value) + '.joblib.pkl' # add extension

    print("Training model on MIT-BIH DS1: " + model_svm_path + "...")

    if os.path.isfile(model_svm_path):
        # Load the trained model!
        svm_model = joblib.load(model_svm_path)

    else:
        svm_model = svm.SVC(C=C_value, kernel='rbf', degree=3, gamma='auto', 
            coef0=0.0, shrinking=True, probability=True, tol=0.001, 
            cache_size=200, class_weight='balanced', verbose=False, max_iter=-1, 
            decision_function_shape='ovo', random_state=None)
        
        # Let's Train!
        svm_model.fit(tr_features_scaled, tr_labels) 
        # TODO assert that the class_ID appears with the desired order, 
        # with the goal of ovo make the combinations properly
        print("Trained completed!\n\t" + model_svm_path)

        # Export model: save/write trained SVM model
        joblib.dump(svm_model, model_svm_path)
    
    ##############################
    ## Test SVM model
    print("Testing model on MIT-BIH DS2: " + model_svm_path + "...")

    [eval_features, eval_labels] = load_mit_db('DS2', winL, winR, do_preprocess, maxRR, use_RR, norm_RR, compute_morph, db_path)
    #np.savetxt('mit_db/DS2_labels.csv', eval_labels.astype(int), '%.0f') 

    # Normalization of the input data
    # scaled: zero mean unit variance ( z-score )
    eval_features_scaled = scaler.transform(eval_features)

    # Let's test new data!
    #predicts_ovo        = svm_model.decision_function(eval_features_scaled)
    # TODO combine these values to get a final prediction!
    #predicts_log_proba  = svm_model.predict_log_proba(eval_features_scaled)
    #predicts_proba      = svm_model.predict_proba(eval_features_scaled)

    predicts            = svm_model.predict(eval_features_scaled)

    print("Evaluation")
    perf_measures = compute_AAMI_performance_measures(predicts, eval_labels)
    
    perf_measures_path = create_svm_model_name('results', winL, winR, do_preprocess, use_RR, norm_RR, compute_morph, use_weight_class, '/')
    # Write results and also predictions on DS2
    if not os.path.exists(perf_measures_path):
        os.makedirs(perf_measures_path)

    write_AAMI_results( perf_measures, perf_measures_path + '/C_' + str(C_value) + '_score_Ijk_' + str(format(perf_measures.Ijk, '.2f')) + '.txt')
    
    # Array to .csv
    np.savetxt(perf_measures_path + '/C_' + str(C_value) + '_predicts.csv', predicts.astype(int), '%.0f') 

    print("Results writed at " )

    
if __name__ == '__main__':
    import sys
    main(sys.argv[1:])
