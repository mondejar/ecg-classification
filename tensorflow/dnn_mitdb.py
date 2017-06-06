""" 
Author: Mondejar Guerra
VARPA
University of A Coruna
April 2017

Description: Train and evaluate mitdb with interpatient split (train/test)
Uses DNN clasifier 
"""

import numpy as np
import matplotlib.pyplot as plt
import os
import csv
import pickle
import numpy as np
import matplotlib.pyplot as plt
import tensorflow as tf
import collections
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

def main():
  window_size = 160
  compute_RR_interval_feature = True
  compute_wavelets = True
  dataset = '/home/mondejar/dataset/ECG/mitdb/'
  output_path = dataset + 'm_learning/'

  # 0 Load Data
  train_data, train_labels, eval_data, eval_labels = load_data(output_path, window_size, compute_RR_interval_feature, compute_wavelets)

  # 1 TODO Preprocess data? norm? if RR interval, last 4 features are pre, post, local and global RR

  # Apply some norm? convolution? another approach?
  
  # [0,33] wave
  normalize = True
  if normalize:
    feature_size = len(train_data[0])
    if compute_RR_interval_feature:
      feature_size = feature_size - 4

    max_wav = np.amax(np.vstack((train_data[:, 0:feature_size], eval_data[:, 0:feature_size])))
    min_wav = np.amin(np.vstack((train_data[:, 0:feature_size], eval_data[:, 0:feature_size])))
    
    train_data[:, 0:feature_size] = ((train_data[:,0:feature_size] - min_wav) / (max_wav - min_wav))

    eval_data[:, 0:feature_size] = ((eval_data[:,0:feature_size] - min_wav) / (max_wav - min_wav))
    #Norm last part feature: RR interval 
    if compute_RR_interval_feature:

      max_rr = np.amax(np.vstack((train_data[:, feature_size:], eval_data[:, feature_size:])))
      min_rr = np.amin(np.vstack((train_data[:, feature_size:], eval_data[:, feature_size:])))

      train_data[:, feature_size:] = ((train_data[:, feature_size:] - min_rr) / (max_rr - min_rr))
      eval_data[:,  feature_size:] = ((eval_data[:, feature_size:] - min_rr) / (max_rr - min_rr))

    # [34,38] RR interval

  # 2 Create model 

  # Specify that all features have real-value data
  feature_columns = [tf.contrib.layers.real_valued_column("", dimension=len(train_data[0]))]

  # Build 3 layer DNN with 10, 20, 10 units respectively.

  mitdb_classifier = tf.contrib.learn.DNNClassifier(feature_columns=feature_columns,
                                              hidden_units=[10, 20, 10],
                                              n_classes=5,
                                              model_dir="/tmp/mitdb")

  # Fit model.
  # Define the training inputs
  def get_train_inputs():
    x = tf.constant(train_data)
    y = tf.constant(train_labels)

    return x, y
    
  mitdb_classifier.fit(input_fn=get_train_inputs, steps=2000)

  # Evaluate accuracy. 
  def get_test_inputs():
    x = tf.constant(eval_data)
    y = tf.constant(eval_labels)
    return x, y

  accuracy_score = mitdb_classifier.evaluate(input_fn=get_test_inputs, steps=1)["accuracy"]
  print("\nTest Accuracy: {0:f}\n".format(accuracy_score))

  def get_eval_data():
    return np.array(eval_data, dtype=np.float32)

  predictions = list(mitdb_classifier.predict(input_fn=get_eval_data))

  # Compute the matrix confussion
  confusion_matrix = np.zeros((5,5), dtype='int')
  for p in range(0, len(predictions), 1):
      confusion_matrix[predictions[p]][eval_labels[p]] = confusion_matrix[predictions[p]][eval_labels[p]] + 1
  
  print confusion_matrix

if __name__ == "__main__":
  main()
