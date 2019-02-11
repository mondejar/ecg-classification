# ECG Classification 

The code contains the implementation of a method for the automatic classification of electrocardiograms (ECG) based on the combination of multiple Support Vector Machines (SVMs). The method relies on the time intervals between consequent beats
and their morphology for the ECG characterisation.  Different descriptors based on wavelets, local binary patterns
(LBP), higher order statistics (HOS) and several amplitude values were employed. 

For a detailed explanation refer to the paper: [http://www.sciencedirect.com/science/article/pii/S1746809418301976](http://www.sciencedirect.com/science/article/pii/S1746809418301976)

If you use this code for your publications, please cite it as:

    @article{MONDEJARGUERRA201941,
    author = {Mond{\'{e}}jar-Guerra, V and Novo, J and Rouco, J and Penedo, M G and Ortega, M},
    doi = {https://doi.org/10.1016/j.bspc.2018.08.007},
    issn = {1746-8094},
    journal = {Biomedical Signal Processing and Control},
    pages = {41--48},
    title = {{Heartbeat classification fusing temporal and morphological information of ECGs via ensemble of classifiers}},
    volume = {47},
    year = {2019}
    }



## Requirements

Python implementation is the most updated version of the repository. Matlab implementation is independent. Both implementations are tested under Ubuntu 16.04. 

### [Python](python)

- [Numpy](https://docs.scipy.org/doc/numpy-1.13.0/user/install.html)
- [Scikit learn](http://scikit-learn.org/stable/install.html)
- [Matplotlib](https://matplotlib.org/) (Optional)
        
### [Matlab](matlab)
Performed using Matlab 2016b 64 bits 

- [LibSVM](https://www.csie.ntu.edu.tw/~cjlin/libsvm/#download)

*Implementation for [TensorFlow](tensorflow) is in early stage and will not be maintained by the author.*


## Steps (How to run)
 
1. Download the dataset:
    - a) Download via Kaggle:

        The raw signals files (.csv) and annotations files can be downloaded from [kaggle.com/mondejar/mitbih-database](https://www.kaggle.com/mondejar/mitbih-database)

    - b) Download via WFDB:

        https://www.physionet.org/faq.shtml#downloading-databases

        Using the comand **rsync** you can check the datasets availability:

        ```
        rsync physionet.org::
        ```
        The terminal will show all the available datasets:
        ```
        physionet      	PhysioNet web site, volume 1 (about 23 GB)
        physionet-small	PhysioNet web site, excluding databases (about 5 GB)
        ...
        ...
        umwdb          	Unconstrained and Metronomic Walking Database (1 MB)
        vfdb           	MIT-BIH Malignant Ventricular Ectopy Database (33 MB)
        ```

        Then select the desired dataset as:
        ```
        rsync -Cavz physionet.org::mitdb /home/mondejar/dataset/ECG/mitdb
        ```

        ```
        rsync -Cavz physionet.org::incartdb /home/mondejar/dataset/ECG/incartdb
        ```

        Finally to convert the data as plain text files use [convert_wfdb_data_2_csv.py](https://github.com/mondejar/WFDB_utils_and_others/blob/master/convert_wfdb_data_2_csv.py). One file with the raw data and one file for annotations ground truth. 

        Also check the repo [WFDB_utils_and_others](https://github.com/mondejar/WFDB_utils_and_others) for more info about WFDB database conversion and the original site from [Physionet_tools](https://www.physionet.org/physiotools/wag/wag.htm).

2. Run:

    Run the file *run_train_SVM.py* and adapt the desired configuration to call *train_SVM.py* file. This call method will train the SVM model using the training set and evaluates the model on a different test set. 

    Check and adjust the path dirs on *train_SVM.py* file.

4. Combining multiples classifiers:
    
    Run the file *basic_fusion.py* to combine the decisions of previously trained SVM models. 


## Methodology

The data is splited following the **inter-patient** scheme proposed by [Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)., i.e the training and eval set not contain any patient in common.

This code classifies the signal at beat-level following the class labeling of the [AAMI recomendation](###aami-recomendation-for-mit). 

### 1 Preprocess:
First, the baseline of the signal is substracted. Additionally, some noise removal can be done.

Two median filters are applied for this purpose, of 200-ms and 600-ms.
Note that this values depend on the frecuency sampling of the signal.


```python
    from scipy.signal import medfilt
    ...
    
    # median_filter1D
    baseline = medfilt(MLII, 71) 
    baseline = medfilt(baseline, 215) 
```
            
The signal resulting from the second filter operation contains the baseline wanderings and can be subtracted from the original signal.
```python        
    # Remove Baseline
    for i in range(0, len(MLII)):
        MLII[i] = MLII[i] - baseline[i]
```

### 2 Segmentation: Beat Detection
In this work the annotations of the MIT-BIH arrhyhtmia was used in order to detect the R-peak positions. However, in practise they can be detected using the following software (see [Software references: Beat Detection](#software-references:-beat-detection)). 

### 3 Feature Descriptor
In order to describe the beats for classification purpose, we employ the following  features:

1. **Morphological**: for this features a window of [-90, 90] was centred along the R-peak:

    0. **RAW-Signal** (180): is the most simplier descriptor. Just employ the amplitude values from the signal delimited by the window.
    
    1. **Wavelets** (23): The wavelet transforms have the capability to allow information extraction from both frequency and time domains, which make them suitable for ECG description. The signal is decomposed using *wave_decomposition* function using family *db1*  and 3 levels. 

    ```python
        import pywt
        ...

        db1 = pywt.Wavelet('db1')
        coeffs = pywt.wavedec(beat, db1, level=3)
        wavel = coeffs[0]
    ```

    2. **HOS** (10): extracted from 3-4th order cumulant, skewness and kurtosis. 
    ```python
        import scipy.stats
        ...
        
        n_intervals = 6
        lag = int(round( (winL + winR )/ n_intervals))
        ...
        # For each beat 
        for i in range(0, n_intervals-1):
            pose = (lag * (i+1))
            interval = beat[(pose -(lag/2) ):(pose + (lag/2))]
            # Skewness  
            hos_b[i] = scipy.stats.skew(interval, 0, True)

            # Kurtosis
            hos_b[5+i] = scipy.stats.kurtosis(interval, 0, False, True)
    ```
    3. **U-LBP 1D (59)** 1D version of the popular LBP descriptor. Using the uniform patterns with neighbours = 8
    ```python
        import numpy as np
        ...

        hist_u_lbp = np.zeros(59, dtype=float)

        for i in range(neigh/2, len(signal) - neigh/2):
            pattern = np.zeros(neigh)
            ind = 0
            for n in range(-neigh/2,0) + range(1,neigh/2+1):
                if signal[i] > signal[i+n]:
                    pattern[ind] = 1          
                ind += 1
            # Convert pattern to id-int 0-255 (for neigh =8)
            pattern_id = int("".join(str(c) for c in pattern.astype(int)), 2)

            # Convert id to uniform LBP id 0-57 (uniform LBP)  58: (non uniform LBP)
            if pattern_id in uniform_pattern_list:
                pattern_uniform_id = int(np.argwhere(uniform_pattern_list == pattern_id))
            else:
                pattern_uniform_id = 58 # Non uniforms patternsuse

            hist_u_lbp[pattern_uniform_id] += 1.0
    ```

    4. **My Descriptor (4)**: computed from the Euclidean distance of the  R-peak  and  four  points  extracted  from  the 4 following intervals:
        - max([0, 40])
        - min([75, 85])
        - min([95, 105])
        - max([150, 180])

    ```python
        import operator
        ...

        R_pos = int((winL + winR) / 2)

        R_value = beat[R_pos]
        my_morph = np.zeros((4))
        y_values = np.zeros(4)
        x_values = np.zeros(4)
        # Obtain (max/min) values and index from the intervals
        [x_values[0], y_values[0]] = max(enumerate(beat[0:40]), key=operator.itemgetter(1))
        [x_values[1], y_values[1]] = min(enumerate(beat[75:85]), key=operator.itemgetter(1))
        [x_values[2], y_values[2]] = min(enumerate(beat[95:105]), key=operator.itemgetter(1))
        [x_values[3], y_values[3]] = max(enumerate(beat[150:180]), key=operator.itemgetter(1))
        
        x_values[1] = x_values[1] + 75
        x_values[2] = x_values[2] + 95
        x_values[3] = x_values[3] + 150
        
        # Norm data before compute distance
        x_max = max(x_values)
        y_max = max(np.append(y_values, R_value))
        x_min = min(x_values)
        y_min = min(np.append(y_values, R_value))
        
        R_pos = (R_pos - x_min) / (x_max - x_min)
        R_value = (R_value - y_min) / (y_max - y_min)
                    
        for n in range(0,4):
            x_values[n] = (x_values[n] - x_min) / (x_max - x_min)
            y_values[n] = (y_values[n] - y_min) / (y_max - y_min)
            x_diff = (R_pos - x_values[n]) 
            y_diff = R_value - y_values[n]
            my_morph[n] =  np.linalg.norm([x_diff, y_diff])
    ```

2. **Interval RR** (4): intervals computed from the time between consequent beats. There are the most common feature employed for ECG classification. 
    1. pre_RR
    2. post_RR
    3. local_RR
    4. global_RR

3. **Normalized RR** (4): RR interval normalized by the division with the AVG value from each patient.
    1. pre_RR / AVG(pre_RR)
    2. post_RR / AVG(post_RR)
    3. local_RR / AVG(local_Python (Scikit-learn)  
    4. global_RR / AVG(global_RR)   

 **NOTE**: 
 *Beats having a R–R interval smaller than 150 ms or higher than 2 s most probably involve segmentation errors and are discarded*. "Weighted Conditional Random Fields for Supervised Interpatient Heartbeat Classification"* 

### 4 Normalization of the features

Before train the models. All the input data was standardized with [z-score](https://en.wikipedia.org/wiki/Standard_score), i.e., the values
of each dimension are divided by its standard desviation and substracted by its mean.
```python
    import sklearn
    from sklearn.externals import joblib
    from sklearn.preprocessing import StandardScaler
    from sklearn import svm
    ...

    scaler = StandardScaler()
    scaler.fit(tr_features)
    tr_features_scaled = scaler.transform(tr_features)

    # scaled: zero mean unit variance ( z-score )
    eval_features_scaled = scaler.transform(eval_features)
```

### 5 Training and Test

In scikit-learn the multiclass SVM support is handled according to a [one-vs-one](http://scikit-learn.org/stable/modules/generated/sklearn.svm.SVC.html) scheme.

Since   the   MIT-BIH   database presents  high  imbalanced  data,  several  weights
equal  to the  ratio  between  the  two  classes  of  each model  were
employed to compensate this differences.

The  Radial  Basis  Function  (RBF) kernel was employed.

```python
    class_weights = {}
    for c in range(4):
        class_weights.update({c:len(tr_labels) / float(np.count_nonzero(tr_labels == c))})


    svm_model = svm.SVC(C=C_value, kernel='rbf', degree=3, gamma='auto', 
                    coef0=0.0, shrinking=True, probability=use_probability, tol=0.001, 
                    cache_size=200, class_weight=class_weights, verbose=False, 
                    max_iter=-1, decision_function_shape=multi_mode, random_state=None)

    svm_model.fit(tr_features_scaled, tr_labels) 
```


For evaluating the model, the jk index [Mar et. al](https:doi.org/10.1109/TBME.2011.2113395)) were employed as performance measure

```python

    decision_ovo        = svm_model.decision_function(eval_features_scaled)
    predict_ovo, counter    = ovo_voting_exp(decision_ovo, 4)

    perf_measures = compute_AAMI_performance_measures(predict_ovo, labels)
```

### 6 Combining Ensemble of SVM

Several basic combination rules can be employed to combine the decision from different SVM model configurations in a single prediction (see basic_fusion.py)


### 7 Comparison with state-of-the-art on MITBIH database:


| Classifier          | Acc. | Sens. | jk index |
|---------------------|------|-------|----------|
| Our Ensemble of SVMs| **0.945**| 0.703 | **0.773** |
| [Zhang et al.](https://doi.org/10.1016/j.compbiomed.2013.11.019)        | 0.883| **0.868** | 0.663 |
| Out Single SVM      | 0.884| 0.696 | 0.640 |
| [Mar et al.](https://doi.org/10.1109/TBME.2011.2113395)| 0.899| 0.802 | 0.649 |
| [Chazal et al.](https://doi.org/10.1109/TBME.2004.827359)       | 0.862| 0.832 | 0.612 |

# About datasets:

https://physionet.org/cgi-bin/atm/ATM

# MIT-Arrythmia Database

<b>360HZ</b>

48 Samples of 30 minutes, 2 leads 
47 Patients:

* 100 series: 23 samples
* 200 series: 25 samples. **Contains uncommon but clinically important arrhythmias**

| Symbol|   Meaning                                   |
|-------|---------------------------------------------|
|· or N |	Normal beat                                 |
|L      |   Left bundle branch block beat             |
|R      |	Right bundle branch block beat              |
|A      |	Atrial premature beat                       |
|a      |	Aberrated atrial premature beat             |
|J      |	Nodal (junctional) premature beat           |
|S      |	Supraventricular premature beat             |
|V      |	Premature ventricular contraction           |
|F      |	Fusion of ventricular and normal beat       |
|[      |	Start of ventricular flutter/fibrillation   |
|!      |	Ventricular flutter wave                    |
|]      |	End of ventricular flutter/fibrillation     |
|e      |	Atrial escape beat                          |
|j      |	Nodal (junctional) escape beat              |
|E      |	Ventricular escape beat                     |
|/      |	Paced beat                                  |
|f      |	Fusion of paced and normal beat             |
|x      |	Non-conducted P-wave (blocked APB)          |
|Q      |	Unclassifiable beat                         |
||      |	Isolated QRS-like artifact                  |

[beats and rhythms](https://physionet.org/physiobank/database/html/mitdbdir/tables.htm#allrhythms)

||Rhythm annotations appear below the level used for beat annotations|
|-|-------------------------------------------------------------------|
|(AB |	    Atrial bigeminy|
|(AFIB |	Atrial fibrillation|
|(AFL|	Atrial flutter|
|(B|	    Ventricular bigeminy|
|(BII|	2° heart block|
|(IVR|	Idioventricular rhythm|
|(N|	    Normal sinus rhythm|
|(NOD|	Nodal (A-V junctional) rhythm|
|(P|	    Paced rhythm|
|(PREX|	Pre-excitation (WPW)|
|(SBR|	Sinus bradycardia|
|(SVTA|	Supraventricular tachyarrhythmia|
|(T|	    Ventricular trigeminy|
|(VFL|	Ventricular flutter|
|(VT|	    Ventricular tachycardia

### AAMI recomendation for MIT 
There are 15 recommended classes for arrhythmia that are classified into 5 superclasses: 

| SuperClass| | | | | | | 
|------|--------|---|---|---|---|-|
| N  (Normal)  | N      | L | R |  |  | |
| SVEB (Supraventricular ectopic beat) | A      | a | J | S |  e | j |
| VEB  (Ventricular ectopic beat)| V      | E |   |   |   | |
| F    (Fusion beat) | F      |   |   |   |   | |
| Q   (Unknown beat)  | P      | / | f | u |   |    |

### Inter-patient train/test split ([Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)):
DS_1 Train: 101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230

| Class|N|SVEB|VEB|F|Q|
|------|-|-|-|-|-|
| instances| 45842|944|3788|414|0|    
 
DS_2 Test: = 100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234

| Class|N|SVEB|VEB|F|Q|
|------|-|-|-|-|-|
| instances| 44743|1837|3447|388|8|    
 
# INCART Database 
https://www.physionet.org/pn3/incartdb/

<b>257HZ</b>

75 records of 30 minutes, 12 leads [-4000, 4000]

Gains varying from 250 to 1100 analog-to-digital converter units per millivolt.
Gains for each record are specified in its .hea file. 

The reference annotation files contain over 175,000 beat annotations in all.

The original records were collected from patients undergoing tests for coronary artery disease (17 men and 15 women, aged 18-80; mean age: 58). None of the patients had pacemakers; most had ventricular ectopic beats. In selecting records to be included in the database, preference was given to subjects with ECGs consistent with ischemia, coronary artery disease, conduction abnormalities, and arrhythmias;observations of those selected included:



# Software references: Beat Detection
1. [*Pan Tompkins*](https://es.mathworks.com/matlabcentral/fileexchange/45840-complete-pan-tompkins-implementation-ecg-qrs-detector)
    
    [third_party/Pan_Tompkins_ECG_v7/pan_tompkin.m](third_party/Pan_Tompkins_ECG_v7/pan_tompkin.m)

2. [*ecgpuwave*](third_party/README.md) Also gives QRS onset, ofset, T-wave and P-wave
NOTE:*The beats whose Q and S points were not detected are considered as outliers and automatically rejected from our datasets.*

3. [ex_ecg_sigprocessing](https://es.mathworks.com/help/dsp/examples/real-time-ecg-qrs-detection.html)

4. osea


# License
The code of this repository is available under [GNU GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.html).
