TensorFlow implementation of ecg classification. 

# Prepare data
To prepare the dataset *create_traindataset_mitdb.py* extract the beats from all patients, compute the RR interval information and set their corresponding label from the annotation files.

# Models

## DNN classifier
In *dnn_mitdb.py* a DNN default classifier from tensorflow is used

```python
    mitdb_classifier = tf.contrib.learn.DNNClassifier(feature_columns=feature_columns,
    hidden_units=[10, 20, 10],
    n_classes=5)
```

## My own model classifier
Due to the imbalanced data (common in that problem) between N class and anomalies class (SVEB, VEB, F). In *my_dnn_mitdb.py* a classifier that adjust the weight for loss computation during training step is defined. 

```python
def my_model_fn(features, targets, mode, params):
    ...
    loss = tf.losses.softmax_cross_entropy(targets_onehot, output_layer, weights=weights_tf)
    ...

my_nn = tf.contrib.learn.Estimator(model_fn=my_model_fn, params=model_params)
```

# Requirements

[Installation guide](installation_guide.md)

Tensorflow

python-matplotlib

pywavelets
