from sklearn.feature_selection import SelectFromModel
from sklearn.linear_model import LassoCV
from sklearn.feature_selection import SelectPercentile, f_classif
from sklearn.feature_selection import SelectKBest
from sklearn.feature_selection import chi2

import numpy as np
# https://machinelearningmastery.com/feature-selection-machine-learning-python/

def run_feature_selection(features, labels, feature_selection, best_features):
    
    if feature_selection == 'select_K_Best':
        # feature extraction
        selector = SelectKBest(score_func=f_classif, k=4) # score_func=chi2 : only for non-negative features
        selector.fit(features, labels)
        # summarize scores
        scores = selector.scores_
        features_index_sorted = np.argsort(-scores)
        features_selected = features[:, features_index_sorted[0:best_features]]

    # SelectFromModel and LassoCV
    
    # We use the base estimator LassoCV since the L1 norm promotes sparsity of features.
    if feature_selection == 'LassoCV':
        clf = LassoCV()

        # Set a minimum threshold of 0.25
        sfm = SelectFromModel(clf, threshold=0.95)
        sfm.fit(features, labels)
        features_selected = sfm.transform(features).shape[1]

        """
        # Reset the threshold till the number of features equals two.
        # Note that the attribute can be set directly instead of repeatedly
        # fitting the metatransformer.
        while n_features > 2:
            sfm.threshold += 0.1
            X_transform = sfm.transform(X)
            n_features = X_transform.shape[1]
        """

    # Univariate feature selection
    # Univariate feature selection works by selecting the best features based on univariate statistical tests. 
    # It can be seen as a preprocessing step to an estimator. 
    # Scikit-learn exposes feature selection routines as objects that implement the transform method:
    #   - SelectKBest removes all but the k highest scoring features
    #   - SelectPercentile removes all but a user-specified highest scoring percentage of features
    #       common univariate statistical tests for each feature: false positive rate SelectFpr, false discovery rate SelectFdr, or family wise error SelectFwe.
    #   - GenericUnivariateSelect allows to perform univariate feature selection with a configurable strategy. This allows to select the best univariate selection strategy with hyper-parameter search estimator.

    if feature_selection == 'slct_percentile':
        selector = SelectPercentile(f_classif, percentile=10)
        selector.fit(features, labels)
        # The percentile not affect. 
        # Just select in order the top features by number or threshold

        # Keep best 8 values?
        scores = selector.scores_
        features_index_sorted = np.argsort(-scores)
        # scores = selector.scores_

        # scores = -np.log10(selector.pvalues_)
        # scores /= scores.max()

        features_selected = features[:, features_index_sorted[0:best_features]]

    print("Selected only " + str(features_selected.shape) + " features ")

    return features_selected, features_index_sorted