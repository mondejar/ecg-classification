# ECG Classification 

## Description
Code for training and test **MIT-BIH Arrhythmia Database** with:
1. [Support Vector Machine (**SVM**) on Python](matlab/README.md).
2. [Support Vector Machine (**SVM**) on MATLAB (*old*)](matlab/README.md).
3. [Artificial Neural Networks (**ANNs**) on TensorFlow (*old*)](tensorflow/README.md)

The data is splited following the **inter-patient** scheme proposed by Chazal et al [Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)., i.e the training and eval set not contain any patient in common.

## Method
This code analyzes the state of the beats with the four classes of the [AAMI recomendation](### AAMI recomendation form MIT)). 

### 1 Beat Detection
In this work the annotations of the MIT-BIH arrhyhtmia was used for the R-peak positions. However it can be detected using the following software: 

1. *third_party/pan_tompkin.m*. 

2. [*ecgpuwave*](third_party/README.md) Also gives QRS onset, ofset, T-wave and P-wave
NOTE:*The beats whose Q and S points were not detected are considered as outliers and automatically rejected from our datasets.*

3. [ex_ecg_sigprocessing](https://es.mathworks.com/help/dsp/examples/real-time-ecg-qrs-detection.html)

4. osea

Then, once the signal is spllited into beats, several features can be computed: 

### 2 Preprocess:
Usually involves some band-pass filtering to remove noise from signal. 
It will depends on the frecuency sampling.

Two median filters are applied for this purpose, of 200-ms and 600-ms.
The signal resulting from the second filter operation contains the baseline wanderings and can be subtracted from the original signal.

### 3 Feature Descriptor

The inputs of the machine learning methods are form by a combinations of these features:

1. **Morphological**: for this features typically a window is centered along the R-peak:

    0. **RAW** signal
    
    1. **Wavelets** (25): The signal is decomposed using *wave_decomposition* function using family *db8*  and 4 levels. 

    3. **HOS** (): extracted from 2-3-4th order cumulant.
    Python: https://docs.scipy.org/doc/scipy-0.19.0/reference/generated/scipy.stats.kstat.html


2. **Interval RR** (4): information extracted about the rhythm 
    1. pre_RR
    2. post_RR
    3. local_RR
    4. global_RR

3. **Normalized RR** (4): normalized the RR by division with the AVG value from the patient.
    1. pre_RR / AVG(pre_RR)
    2. post_RR / AVG(post_RR)
    3. local_RR / AVG(local_RR) 
    4. global_RR / AVG(global_RR)   

 ***NOTA** TODO: Beats having a R–R interval smaller than 150 ms or higher than 2 s most probably involve segmentation errors and are discarded. Extracted from: Weighted Conditional Random Fields for Supervised Interpatient Heartbeat Classification* 


### Normalization of the features



## Top-Papers:

### [Weighted SVMs and Feature Relevance Assessment in Supervised Heart Beat Classification] 2010

### [Electrocardiogram Classification Using Reservoir Computing With Logistic Regression] 2015
(http://ieeexplore.ieee.org/document/6840304/)
Cites: 10   (RETIRADO DE LA REVISTA)
        
        Best method in the survey: *ECG-based heartbeat classification for arrhythmia detection: A survey*

### [ECG-based heartbeat classification for arrhythmia detection: A survey] 2016
(http://www.sciencedirect.com/science/article/pii/S0169260715003314) Cites: 18


### [Automatic Classification of Heartbeats Using ECG Morphology and Heartbeat Interval Features] 2004
(http://www.sciencedirect.com/science/article/pii/S0169260715003314) Cites 740


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
  
  100, 101, 103, 104, ..., 124

* 200 series: 25 samples. **Contains uncommon but clinically important arrhythmias**

  200, 201, 203, 205, ..., 234

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

### AAMI recomendation form MIT 
There are 15 recommended classes for arrhythmia that are classified into 5 superclasses: 

| SuperClass| | | | | | 
|------|--------|---|---|---|---|
| N  (Normal)  | N      | L | R | e | j |
| SVEB (Supraventricular ectopic beat) | A      | a | J | S |   |
| VEB  (Ventricular ectopic beat)| V      | E |   |   |   |
| F    (Fusion beat) | F      |   |   |   |   |
| Q   (Unknown beat)  | P      | / | f | u |   |   

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
