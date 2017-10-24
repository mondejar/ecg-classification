# ECG Classification 

## Description
Code for training and test **MIT-BIH Arrhythmia Database** with:
1. [Support Vector Machine (**SVM**) on Python](matlab/README.md).
2. [Support Vector Machine (**SVM**) on MATLAB (*old*)](matlab/README.md).
3. [Artificial Neural Networks (**ANNs**) on TensorFlow (*old*)](tensorflow/README.md)

The data is splited following the **inter-patient** scheme proposed by Chazal et al [Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)., i.e the training and eval set not contain any patient in common.

DS1 = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230]

DS2 = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234]

## Method
This code analyzes the state of the beats with the four classes of the [AAMI recomendation](### AAMI recomendation form MIT)). 

### 1 Beat Detection
The *R peak* is know by the dataset (MIT-DB) or can be detected using the following software: 

1. *third_party/pan_tompkin.m*. 

2. [*ecgpuwave*](third_party/README.md) Also gives QRS onset, ofset, T-wave and P-wave
NOTE:*The beats whose Q and S points were not detected are considered as outliers and automatically rejected from our datasets.*

3. [ex_ecg_sigprocessing](https://es.mathworks.com/help/dsp/examples/real-time-ecg-qrs-detection.html)

4. osea

Then, once the signal is spllited into beats, several features can be computed: 

### 2 Preprocess:
Usually involves some band-pass filtering to remove noise from signal. It will depends on the frecuency sampling.


*- Weighted Conditional Random Fields for Supervised Interpatient Heartbeat Classification:
Two median filters are designed for this purpose. The first median filter is of 200-ms width and removes the QRS complexes
and the P waves. The resulting signal is then processed with a second median filter of 600-ms width to remove the T waves.
The signal resulting from the second filter operation contains the baseline wanderings and can be subtracted from the original signal. Powerline artifacts are then removed from the baseline corrected signal with a 60-Hz band-stop filter.*


### 3 Feature Descriptor

The inputs of the machine learning methods are form by a combinations of these features:

1. **Morphological**: for this features typically a window is centered along the R-peak:

    0. **RAW** signal
    
    1. **Wavelets** (25): The signal is decomposed using *wave_decomposition* function using family *db8*  and 4 levels. 

    2. **Chazal** (18) window centered [-50, 500]. Sampling points at two interval  
    (-50 - 100)ms 10 features at 60hz  
     (150 - 500)ms  8 features at 20hz

    3. **HOS** (): extracted from 2-3-4th order cumulant.
      
        Python: https://docs.scipy.org/doc/scipy-0.19.0/reference/generated/scipy.stats.kstat.html
        
        Matlab: 

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

______
## TOP Results at 12 Jun 2017!!

*Note: using **AAMI recomendation form MIT**. Class N also contains LBBB, RBBB, Atrial escape beat, Nodal (junctional) escape*. 

| Method | N | SVEB | VEB | F | |  AVG_class| Acc_total | AVG_class * Acc_total|
|--------|---|------|-----|---|-|----|--|--|
|Weigted_SVM [Lannoy-2010](https://link.springer.com/chapter/10.1007/978-3-642-18472-7_17) |0.8|0.88|0.78|0.87|| **0.83**| | |
|LD [Chazal-2004](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)|0.86|0.75|0.8|0.89||0.825| 0.8623|0.7437 |
|My SVM| |||||||

*NOTE: chazal no usa exactamente la misma cantidad de Beats que nosotros* 

### SVM Best-Configuration:
One - vs - one 

Kernel: LIN SVM

C = [0.001, 0.1, 1, 10, 50, 100]

Feature set:
1. RR: by deault we divided RR values by 1000 to obtain values in the range [0,1]
2. Normalized RR (AVG RR).  RR_beat / AVG(RR_patient)
3. Wavelet

NOTE from work : ECG Beat Recognition Using Fuzzy Hybrid Neural Network
Each component of  a  cumulant vector has been divided by the maximum value of the corresponding cumulant. In similar way, the other three temporary features have been also normalized. As a result, the input vector coefficients are within the range of zero to unity.

Total instances: 49691 (window size 90,90)

| SVM Configuration                            |N (R/P/FPR)        | S (R/P/FPR)         |  V (R/P/FPR)        | F (R/P/FPR)         |  Acc  | 
|----------------------------------------------|-------            |------               |------               |------               |-------|
|My SVM_W_RR_AVG_RR_wavelets_z_score  C (0.001)|0.7860 0.9944 0.036| 0.8977 0.3308 0.0697| 0.8174 0.7766 0.0163| 0.7010 0.0429 0.1230| 0.7915|
|LD [Chazal-2004]                              |0.87   0.99        | 0.76   0.38         | 0.77   0.82        |                     | 0.83  |


BEST SCORE AT MAR PAPER Ijk: **0.649** (2011)

My SVM_W_RR_AVG_RR_wavelets_z_score  C (0.001) (OLD)
---------------
|  |n    |   s |   v | f   |  
|--|-----|-----|-----|-----|
|N |**34778**| 3093|  648| 5727|
|S |   60|**1649**|  109|   19|
|V |   32|  240| **2632**|  316|
|F |  104|    3|    9|  **272**|


MY TOP SCORE 14 Jul 2017  C(0.01) Mar scores Ijk: 0.610082
-----------------------------------------
wi lin HOS RR AVG_RR std max_RR

|  |n    |   s |   v | f   |  
|--|-----|-----|-----|-----|
|N |**37165**| 3445|  1090| 2546|
|S |   53|**1714**|  61|   9|
|V |   19|  244| **2817**|  140|
|F |  102|    1|    87|  **198**|
Ij: 2.834990 (max 4)
Cohen Kappa Values: 0.511416
   

MY TOP SCORE 14 Jul 2017  C(0.01) Mar scores Ijk: 0.611094
-----------------------------------------
wi lin HOS RR AVG_RR max_RR

|  |n    |   s |   v | f   |  
|--|-----|-----|-----|-----|
|N |**34758**| 3229| 116 |6143|
|S |49| **1683**| 83| 22|
|V |25| 159| **2905**| 131|
|F |15| 3| 33| **337**|


C 2 Ijk: 0.621
wi lin HOS RR AVG_RR std max_RR
Confussion Matrix 
37391 3600 706 2549
83 1695 53 6
25 273 2822 100
95 1 98 194

Ij: 3.085922 (max 4)
Cohen Kappa Values: 0.450707

C 0.0004 Ijk: 0.664
HOS + MyMorph + RR + AVG_RR
Confussion Matrix 
38918 2812 162 2354
177 1558 62 40
25 194 2878 123
133 3 16 236


 
Total instances: 50424

NOTE: if some configuration uses C 100 or C 0.0001 a higher/lower value must be tested
(OLD)

LA MEDIDA FLEISS KAPPA NO CONVENCE, INCLUSO EN RESULTADOS MUY MALOS 

| SVM Configuration                            |  N   | SVEB | VEB  | F    | |AVG_class | Acc    | AVG_class * ACC|
|----------------------------------------------|------|------|------|------|-|----------|--------|----------------|
|My SVM_W_RR_AVG_RR_wavelets          C (0.1)  |0.7930|0.9042|0.8390|0.6392| |0.7938    |0.7990  |**0.6342**|
|My SVM_W_RR_AVG_RR_wavelets_z_score  C (0.001)|0.7872|0.8977|0.8021|0.7010| |**0.7970**|0.7916  |0.6309|
|My SVM_w_RR_AVG_RR                   C (0.1)  |0.7828|0.6538|0.8007|0.9227| |0.79      |0.7811  |0.6170 |
|My_SVM_w_RR_No Norm                  C (0.1)  |0.8307|0.8525|0.7499|0.5567| |0.7474    |0.8238  |0.6157 |  
|My SVM_w_RR_z_score                  C (100)  |0.7749|0.6287|0.7639|0.9562| |0.7809    |0.7702  |0.6014 |
|My SVM_w_RR                          C (100)  |0.768 |0.632 |0.767 |0.956 | |0.7811    |0.7648  |0.5973 |
|My SVM_W_RR_AVG_RR_mean_signal_standar C (0.001)|0.7936|0.7860|0.7354|0.7010| | 0.754  |0.7886  |0.5946|
|My SVM_RR                            C (100)  |0.9842|0.1013|0.7694|0.2784| |0.5333    |0.93    |0.49   |
|My SVM_w_RR_unit                     C (0.01) |0.8216|0.6668|0.7676|0.0129| |0.5672    |0.8060  |0.4571 |
|My SVM_w_avg_RR                      C (0.1)  |0.7946|0.5879|0.799 |0.0206| |0.5505    |0.7814  |0.4301 |
|My SVM_w_HOS                         C ()     |      |      |      |      | |          |        |       |     
|My SVM_w_wavelets_db1                C ()     |0.6102|0.6603|0.2321|0.7139| |0.5541    |        |       |
|My SVM_w_wavelets_db8                C (10)   |0.0   |0.854 |0.8129|0.0   | |0.4169    |        |       |
|My SVM_w_LIN_50                               |0.75  | 0.82 |0.79  |0.67  | |0.7500    |        |       |
|                                              |      |      |      |       ||        |       |       |
|My SVM My descriptor               C (0.1)   |       |     |   |     |     |   |     |   |

My SVM_W_RR_AVG_RR_wavelets_z_score_RBF C(1) gamma(0.05) 
0.8816      0.3136    0.8831      0.0438      0.5305     0.8546

My SVM_W_RR_AVG_RR_wavelets_z_score_RBF C(1) gamma(0.5)
0.9973      0.0005    0.0015      0           0.2498     0.8852

**NOTE**:*Consider that the class are **imbalanced**! Is not the same an increment of 5% over F than over SVEB or N.*

### Using 2-class
| Method        | N  |  A | |    |
|---------------|----|----|-|----|
|My SVM_w C 0.01|0.92|0.92| |    |

|Model|2-Class|5-Class|
|-----|-------|-------|
|SVM  | |  0.89 ( 0.94 , 0.03  ,  0.85 , 0.0)|
|SVM_weight  | | |
|NN  | 0.92 | 0.91 ( 0.97 , 0.01 , 0.7, 0.0)|
|NN_weight  | 0.74 | 0.81 ( 0.76, 0.17, 0.82, 0.07 )|
### Confussion-matrix

#### LD [Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)

| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N     | **38444** |      173    |       117    |   33|
|SVEB  |    1904      |    **1395** |     321      |   1|
|VEB   |   303        |      252    |   **2504**   |   7|
|F     |   3509       |      16     |     176      |  **347**|
|acc   |  0.86        |    0.75     |    0.80      |    0.89    |

*NOTE: Neural nets varys too much with the same configuration...



## INCART
Results on Incart. 
1. Resampled to 360 Hz (257Hz)



# OLD
#### NN
*LEARNING_RATE = 0.001 step = 2000   1. fc(64) 2. fc(32) 3. relu(16)*

Classes = 5

Acc = 0.91

| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N   |**42968** | 1632 | 881  | 311 |
|SVEB|    59 |  **27** |    36  |   0 |
|VEB |   1201 |    178 |  **2274**  |   77 |
|F   |    0 |    0 |    27  |  **0** |
|acc|   0.97    |   0.01   |   0.7    |   0.0     |
 
Classes = 2

Acc = 0.92

||N|A|
|--|--|--|
|N|**43544**|  3329|
|A|693 | **2116**|
|acc|    0.98   |  0.38   |      

#### NN_weight
*LEARNING_RATE = 0.001 step = 2000   1. fc(64) 2. fc(32) 3. relu(16)*

Acc = 0.74

| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N   | **33832** | 648 |  308 |  185|
|SVEB| 3998  |**313**  | 114  |   3  |
|VEB | 4773  |864 |   **2645**|   172 |
|F   | 1634  |   11 |  153|   **28**  |
|acc|   0.76    | 0.17     | 0.82      |  0.07      |

Classes = 2

Acc = 0.81

||N|A|
|--|--|--|
|N|**35872**|  1096|
|A|8365 | **4349**|
|acc|   0.81    |   0.79   |

#### SVM
*C = 32, gamma = 0.05*

acc = 0.6252

| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N   |**34596**|      369  |      2159   |      363|
|SVEB|9810    |    **1434**   |      796    |      14|
|VEB |337     |     34    |    **492**     |     11|
|F   |0       |    0      |     0       |    **0**|
|acc |  0.77      |   0.78        |     0.14       |     0.0      |


*C = 64, gamma = 0.05*

acc = 0.7751

| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N   |**42275**   |     1446  |       469  |       355 |
|SVEB|199     |     **79**    |      49    |       2   |
|VEB |1569    |     302   |     **2925**   |       30  |
|F   |700     |     10    |       4    |       **1**   |
|acc |  0.94      |    0.04       |    0.84        |      0.0     |


#### SVM_weight
*C = 64, gamma = 0.05*

acc = 0.7014

| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N     |    **37420**  |      1109  |       219    |     267|
|SVEB     |    868    |    **520**    |     113      |     5|
|VEB     |   2463    |     167    |   **2978**      |    56|
|F     |   3992    |      41    |     137      |   **60**|
|acc |  0.83      |    0.28      |    0.86        |      0.15    |

## Chazal features morphology

#### SVM 
Chazal features morphology 3A

norm_raw_signal, interval_rr, norm_RR, weight, 
44541        1501         609         329
    0           0           7           0
  194         336        2809          59
   16           0          23           0

#### SVM_weight
Chazal features morphology 3A

norm_raw_signal, interval_rr, norm_RR, weight, 
42510        1064         122         260
 1161         431         451           2 
  262         326        2795          75
  818          16          80          51

norm_raw_signal, norm_features, interval_rr, norm_RR, weight, 

39006        1154         159         330
  931         382         229           2
 1615         202        3031          46
 3199          99          29          10

interval_rr, weight
___

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
