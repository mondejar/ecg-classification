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


#def nn_model_fn(features, labels, mode):

def main(unused_argv):
  window_size = 160
  compute_RR_interval_feature = True
  compute_wavelets = True
  dataset = '/home/mondejar/dataset/ECG/mitdb/'
  output_path = dataset + 'm_learning/'

  # 0 Load Data
  train_data, train_labels, eval_data, eval_labels = load_data(output_path, window_size, compute_RR_interval_feature, compute_wavelets)
  
  # 1 TODO Preprocess data? norm? if RR interval, last 4 features are pre, post, local and global RR
  
  # 2 Create model

  # Create the Estimator
     # mnist_classifier = learn.Estimator(
     #    model_fn=cnn_model_fn, model_dir="/tmp/mnist_convnet_model")



  # Set up logging for predictions
  # Log the values in the "Softmax" tensor with label "probabilities"
#  tensors_to_log = {"probabilities": "softmax_tensor"}
#  logging_hook = tf.train.LoggingTensorHook(
#      tensors=tensors_to_log, every_n_iter=50)


  # 3 Train model



  # 4  Train the model
  #mnist_classifier.fit(
  #    x=train_data,
  #    y=train_labels,
  #    batch_size=100,
  #    steps=20000,
  #    monitors=[logging_hook])






  # 5 Configure the accuracy metric for evaluation
 # metrics = {
 #     "accuracy":
 #         learn.MetricSpec(
 #             metric_fn=tf.metrics.accuracy, prediction_key="classes"),
 # }

  # 6  Evaluate the model and print results
 # eval_results = mnist_classifier.evaluate(
 #     x=eval_data, y=eval_labels, metrics=metrics)
 # print(eval_results)







if __name__ == "__main__":
  tf.app.run()
