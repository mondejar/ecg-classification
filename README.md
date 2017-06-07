# ECG Classification 

## Description
Code for training and test **MIT-BIH Arrhythmia Database** with:
1. [Artificial Neural Networks (**ANNs**) on TensorFlow](tensorflow/README.md)
2. [Support Vector Machine (**SVM**) on MATLAB](matlab/README.md).

The data is splited in training and eval sets in an **inter-patient** way, i.e the training and eval set not contain any patient in common, as proposed in the work of [Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)

## Method
This code analyzes the state of the beats with 5, 2 classes (see [AAMI recomendation form MIT](### AAMI recomendation form MIT)). A window of 160 is centered on *R peak* which is know by the dataset. 
The peaks can be detected using the code *third_party/pan_tompkin.m*. 

Feature:

The inputs of the machine learning methods are form by 34 dimensional features:

1. **Wavelets** (30): The signal is decomposed using *wave_decomposition* function using family *db8*  and 4 levels. 

2. **Interval RR** (4): information extracted about the rhythm 
    1. pre_RR
    2. post_RR
    3. local_RR
    4. global_RR

Preprocess:

Normalization

## Results

### Accuracy 

*Note: using **AAMI recomendation form MIT**. Class N also contains some anomalies*

To handle the imbalanced data we also prove setting the weights depending on class in the loss step.

|Model|2-Class|5-Class|
|-----|-------|-------|
|SVM  | |  0.89 ( 0.94 , 0.03  ,  0.85 , 0.0)|
|SVM_weight  | | |
|NN  | 0.92 | 0.91 ( 0.97 , 0.01 , 0.7, 0.0)
|NN_weight  | 0.74 | 0.81 ( 0.76, 0.17, 0.82, 0.07 )|

### Confussion-matrix

*NOTE: Neural nets varys too much with the same configuration...

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




____

## About dataset

### MIT-BIH Arrhythmia 

| Symbol|   Meaning                                     |
|-------|-----------------------------------------------|
|· or N |	Normal beat                                 |
|L      |   Left bundle branch block beat               |
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
 

