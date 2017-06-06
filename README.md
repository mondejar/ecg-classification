# ECG Classification on Tensorflow

## Description
Code for training and test **MIT-BIH Arrhythmia Database** with Artificial Neural Networks (**ANNs**) using *Tensorflow*.

The data is splited in training and eval sets in an **inter-patient** way, i.e the training and eval set not contain any patient in common, as proposed in the work of [Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)

## Models

### DNN classifier
In dnn_mitdb.py a DNN default classifier from tensorflow is used

### My own model classifier
Due to the imbalanced data between N class and anomalies class (V, F) (common in that problem). In my_dnn_mitdb.py a classifier that adjust the weight for loss computation during training step is defined. 


## Requirements

[Installation guide](installation_guide.md)

Tensorflow

python-matplotlib

pywavelets



## About dataset

# MIT-BIH Arrhythmia 

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

## AAMI recomendation form MIT 
There are 15 recommended classes for arrhythmia that are classified into 5 superclasses: 

- Normal                        (N) 
- Supraventricular ectopic beat (SVEB) 
- Ventricular ectopic beat      (VEB) 
- Fusion beat                   (F) 
- Unknown beat                  (Q)

|      |   |        |   |   |   |   |
|------|---|--------|---|---|---|---|
| N    |   | N      | L | R | e | j |
| SVEB |   | A      | a | J | S |   |
| VEB  |   | V      | E |   |   |   |
| F    |   | F      |   |   |   |   |
| Q    |   | P      | / | f | u |   |   


## Inter-patient train/test split ([Chazal *et al*](http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1306572)):
DS_1 Train: 101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230
|      |        |
|------|--------|
|N     |   45842|
|SVEB  |     944|   
|VEB   |    3788|      
|F     |     414|   
|Q     |       0|    
 
DS_2 Test: = 100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234
 
|      |        |
|------|--------|
|N     |   44743|
|SVEB  |    1837|   
|VEB   |    3447|      
|F     |     388|   
|Q     |    8005|    
 

