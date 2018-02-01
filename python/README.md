# Files

## section

sickit learn

Scikit-learn: Machine Learning in Python, Pedregosa et al., JMLR 12, pp. 2825-2830, 2011.

imbalanced-learn

pip install -U imbalanced-learn

# Times

## Required time for training SVMs

Python (Scikit-learn)  
| Configuration | Feature Size | Time (seconds) with prob | Time (seconds) no prob |
|---------------|--------------|--------------------------|------------------------|
| RR_normRR     |  8           |       590.91   |       133.50 sec / (maxRR) 92.88 |
| Wavelets      |  21          |      1503.19   |       193.01 sec                 |
| HOS_myMoprh   |  15          |      1004.50   |       158.19 sec                 |  
| RR_Wav_HOS_myMorph| 44        |                 |  264.69 sec | 

MATLAB (LibSVM) without -b option

| Configuration | Feature Size | -b = True Time (seconds) |Time (seconds) |
|---------------|--------------|--------------------------|---------------|
| RR_normRR     |  8           |                          | 117.4789      |
| Wavelets      |  21          |                          | 282.001       |
| HOS_myMoprh   |  15          |                          | 226.9948      |

