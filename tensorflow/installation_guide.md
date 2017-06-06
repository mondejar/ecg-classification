# Installation guide

## Tensorflow:

### 1. by virtual 
(recomended option by tf's developers)
```
$ sudo apt-get install python-pip python-dev python-virtualenv 
```
 Create a virtualenv environment by issuing the following command:

```
$ virtualenv --system-site-packages ~/tensorflow 
```

Activate the virtualenv environment by issuing one of the following commands:
```
$ source ~/tensorflow/bin/activate
```

The preceding source command should change your prompt to the following:

```
(tensorflow)$ 
```
To exit type:

```
(tensorflow)$ deactivate
```

Issue one of the following commands to install TensorFlow in the active virtualenv environment:

```
(tensorflow)$ pip  install --upgrade tensorflow     # for Python 2.7
(tensorflow)$ pip3 install --upgrade tensorflow     # for Python 3.n
(tensorflow)$ pip  install --upgrade tensorflow-gpu # for Python 2.7 and GPU
(tensorflow)$ pip3 install --upgrade tensorflow-gpu # for Python 3.n and GPU
```

### 2. by pip
```
sudo pip install tensorflow
```


## python-matplotlib
To display plots on python

```
sudo apt-get install python-matplotlib
```

## pywavelets

To compute signal wave decomposition
```
pip install PyWavelets
```