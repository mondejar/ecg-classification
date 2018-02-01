# ECG Classification 

Warning: This repository is still not finished by the author.

NOTE: if this code is usefull for you, cite our work as:


## Description
Code for training and test **MIT-BIH Arrhythmia Database** with:
1. [Support Vector Machine (**SVM**) on Python](matlab/README.md).
2. [Support Vector Machine (**SVM**) on MATLAB (*old*)](matlab/README.md).
3. [Artificial Neural Networks (**ANNs**) on TensorFlow (*old*)](tensorflow/README.md)

The data is splited following the **inter-patient** scheme proposed by [Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)., i.e the training and eval set not contain any patient in common.

## Method
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

1. **Morphological**: for this features a window of [-90, 90] is centred along the R-peak:

    0. **RAW-Signal** (180): is the most simplier descriptor. Just employ the amplitude values from the signal delimited by the window.
    
    1. **Wavelets** (23): The wavelet transforms have the capability to allow information extraction from both frequency and time domains, which make them suitable for ECG description. The signal is decomposed using *wave_decomposition* function using family *db1*  and 3 levels. 

    ```python
        import pywt
        ...

        db1 = pywt.Wavelet('db1')
        coeffs = pywt.wavedec(beat, db1, level=3)
        wavel = coeffs[0]
    ```

    3. **HOS** (10): extracted from 3-4th order cumulant.
    ```python
        import scipy.stats
        ...
        
        n_intervals = 6
        lag = int(round( (winL + winR )/ n_intervals))
        ...
        # For each beat 
        for i in range(0, n_intervals-1):
            pose = (lag * i)
            interval = beat[pose:(pose + lag - 1)]
            # Skewness  
            hos_b[i] = scipy.stats.skew(interval, 0, True)
            # Kurtosis
            hos_b[5+i] = scipy.stats.kurtosis(interval, 0, False, True)
    ```

    4. **My Descriptor**: computed from the Euclidean distance of some points against the R-peak


2. **Interval RR** (4): intervals computed from the time between consequent beats. There are the most common feature employed for ECG classification. 
    1. pre_RR
    2. post_RR
    3. local_RR
    4. global_RR

3. **Normalized RR** (4): RR interval normalized by the division with the AVG value from each patient.
    1. pre_RR / AVG(pre_RR)
    2. post_RR / AVG(post_RR)
    3. local_RR / AVG(local_RR) 
    4. global_RR / AVG(global_RR)   

 **NOTE**: 
 *Beats having a R–R interval smaller than 150 ms or higher than 2 s most probably involve segmentation errors and are discarded*. 
 Extracted from: Weighted Conditional Random Fields for Supervised Interpatient Heartbeat Classification* 

### 4 Normalization of the features

Before train the models. All the input data was standardized with [z-score](https://en.wikipedia.org/wiki/Standard_score), i.e., the values
of each dimension are divided by its standard desviation and substracted by its mean.


### 5 Feature Selection

methods and file and line codes...

### 6 Oversampling

methods and file and line codes...

### 7 Cross-Validation

methods and file and line codes...

### 8 Final Training and Test

methods and file and line codes...


# About datasets:

https://physionet.org/cgi-bin/atm/ATM

#### Download via WFDB
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

(Link file .py converter to .csv and annotations in .txt)


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
| instances| 44743|1837|3447|388|8005|    
 
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


# References

* S. Osowski and T. H. Linh, “Ecg beat recognition using fuzzy hybrid neural network,” IEEE Transactions on Biomedical Engineering, vol. 48, no. 11, pp. 1265–1271, Nov 2001.

* de Lannoy G., François D., Delbeke J., Verleysen M. (2011) Weighted SVMs and Feature Relevance Assessment in Supervised Heart Beat Classification. In: Fred A., Filipe J., Gamboa H. (eds) Biomedical Engineering Systems and Technologies. BIOSTEC 2010. Communications in Computer and Information Science, vol 127. Springer, Berlin, Heidelberg

* E. J. da S. Luz, W. R. Schwartz, G. Cmara-Chvez, and D. Menotti, “Ecg-based heartbeat classification for arrhythmia detection: A survey,” Computer Methods and Programs in Biomedicine, vol. 127, no. Supplement C, pp. 144 – 164, 2016

* P. de Chazal, M. O’Dwyer, and R. B. Reilly, “Automatic classification
of heartbeats using ecg morphology and heartbeat interval features,”
IEEE Transactions on Biomedical Engineering, vol. 51, no. 7, pp. 1196–1206, July 2004.

* Z. Zhang, J. Dong, X. Luo, K.-S. Choi, and X. Wu, “Heartbeat classification using disease-specific feature selection,” Computers in Biology and Medicine, vol. 46, no. Supplement C, pp. 79 – 89, 2014

