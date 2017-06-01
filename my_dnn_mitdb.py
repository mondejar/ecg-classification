""" 
Author: Mondejar Guerra
VARPA
University of A Coruna
April 2017
"""

import numpy as np
import matplotlib.pyplot as plt
import os
import csv
import pickle
import numpy as np
import matplotlib.pyplot as plt
import tensorflow as tf

tf.logging.set_verbosity(tf.logging.INFO)

def load_data(output_path, window_size, compute_RR_interval_feature, compute_wavelets):
  extension = '_' + str(window_size)
  if compute_wavelets:
      extension = extension + '_' + 'wv'
  if compute_RR_interval_feature:
      extension = extension + '_' + 'RR'
  extension = extension + '.csv'

  # Load training and eval data
  train_data = np.loadtxt(output_path + 'train_data' + extension, delimiter=",", dtype=float)
  train_labels =  np.loadtxt(output_path + 'train_label' + extension, delimiter=",",  dtype=np.int32)
  eval_data = np.loadtxt(output_path + 'eval_data' + extension, delimiter=",", dtype=float)
  eval_labels = np.loadtxt(output_path + 'eval_label' + extension, delimiter=",",  dtype=np.int32)

  return (train_data, train_labels, eval_data, eval_labels)

def my_input_fn(data, label):
  x = tf.constant(data)
  y = tf.constant(label)
  return x, y

def main():
  window_size = 160
  compute_RR_interval_feature = True
  compute_wavelets = True
  dataset = '/home/mondejar/dataset/ECG/mitdb/'
  output_path = dataset + 'm_learning/'

  # 0 Load Data
  train_data, train_labels, eval_data, eval_labels = load_data(output_path, window_size, compute_RR_interval_feature, compute_wavelets)

  # 1 TODO Preprocess data? norm? if RR interval, last 4 features are pre, post, local and global RR


  # 2 Create model 
  # We can use standard models (tf.contrib.learn.LinearClassifier, 
  #                                         ...  LinearRegressor,
  #                                         ...  DNNClassifier,
  #                                         ...  DNNRegressor
  #            or create our own model 

  # Specify that all features have real-value data
  feature_columns = [tf.contrib.layers.real_valued_column("", dimension=len(train_data[0]))]

  # Build 3 layer DNN with 10, 20, 10 units respectively.
  mitdb_classifier = tf.contrib.learn.DNNClassifier(feature_columns=feature_columns,
                                              hidden_units=[10, 20, 10],
                                              n_classes=5,
                                              model_dir="/tmp/mitdb4")

  # Fit model.
  # Define the training inputs
  def get_train_inputs():
    x = tf.constant(train_data)
    y = tf.constant(train_labels)

    return x, y
    
  mitdb_classifier.fit(input_fn=get_train_inputs, steps=2000)
  # TODO el loss continua bajando con mas de 10000 iteraciones, definir un numero mayor

  # Evaluate accuracy. 
  def get_test_inputs():
    x = tf.constant(eval_data)
    y = tf.constant(eval_labels)
    return x, y

  accuracy_score = mitdb_classifier.evaluate(input_fn=get_test_inputs, steps=1)["accuracy"]
  print("\nTest Accuracy: {0:f}\n".format(accuracy_score))

  # TODO Obtain the confussion matrix

if __name__ == "__main__":
  main()
