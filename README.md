# ECG Classification 

## Description
Code for training and test **MIT-BIH Arrhythmia Database** with:
1. [Artificial Neural Networks (**ANNs**) on TensorFlow](tensorflow/README.md)
2. [Support Vector Machine (**SVM**) on MATLAB](matlab/README.md).

The data is splited in training and eval sets in an **inter-patient** way, i.e the training and eval set not contain any patient in common, as proposed in the work of [Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)

## Method
This code analyzes the state of the beats with 5, 2 classes (see ###AAMI recomendation form MIT). A window of 160 is centered on *R peak* which is know by the dataset. 
The peaks can be detected using the code *third_party/pan_tompkin.m*. 

Feature:

The inputs of the machine learning methods are form by 34 dimensional features:

    30 waves: The signal is decomposed using *wave_decomposition* function using family *db8*  and 4 levels. 

    4 interval RR: information extracted about the rhythm 
        1. pre_RR
        2. post_RR
        3. local_RR
        4. global_RR

Preprocess:

Normalization

## Results

### Accuracy 

*Note: using AAMI recomendation form MIT. Class N also contains some anomalies*

To handle the imbalanced data we also prove setting the weights depending on class in the loss step.

|Model|2-Class|5-Class|
|-----|-------|-------|
|SVM  | | |
|SVM_weight  | | |
|NN  | | |
|NN_weight  | | |

### Confussion-matrix


NN
*LEARNING_RATE = 0.001 step = 2000   1. fc(64) 2. fc(32) 3. relu(16)*

Accuracy = 

Classes = 5
| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N   |**44175** | 1820 | 2629  | 385 |
|SVEB|    4 |  **12** |    4  |   0 |
|VEB |   57 |    3 |  **438**  |   3 |
|F   |    0 |    0 |    0  |  **0** |


Classes = 2

acc 0: 0.98

acc 1: 0.38

global acc = 0.92

||N|A|
|--|--|--|
|N|**43544**|  3329|
|A|693 | **2116**|


NN_weight
*LEARNING_RATE = 0.001 step = 2000   1. fc(64) 2. fc(32) 3. relu(16)*

Accuracy = 

| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N   | **32477** |545| 175|168 |
|SVEB| 5318  |**674**|317|41|
|VEB | 4341  | 607  |**2546**|115
|F| 2101|11 |181|64|**0**|


Classes = 2

acc 0: 0.81

acc 1: 0.79

global acc = 0.81
||N|A|
|--|--|--|
|N|**35872**|  1096|
|A|8365 | **4349**|





SVM *C = 32, gamma = 0.05*

Accuracy = 

| | N | SVEB | VEB | F |
|--|---|--|--|--|
|N | **42281** |1313 |463|355|
|SVEB|196 |**58**| 45|0 |
|VEB|2067|454|**2936**|33|
|F|199|12|3|**0**|





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
 

