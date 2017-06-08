""" 
Author: Mondejar Guerra
VARPA
University of A Coruna
April 2017

Description: Train and evaluate mitdb with interpatient split (train/test)
Uses my own model clasifier with weights for imbalanced class
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
from tensorflow.contrib.learn.python.learn.estimators import model_fn as model_fn_lib

tf.logging.set_verbosity(tf.logging.INFO)

def compute_accuracy(m):
  # Accuracy by column
  classes = m.shape[0]
  acc = np.zeros(classes)
  acc_global = 0
  for c in range(0, classes):
    if sum(m[:,c]) > 0:
      acc[c] = float(m[c,c]) / float(sum(m[:,c]))
    acc_global = acc_global + m[c,c]
    #print ('acc ' + str(c) + ': ' + str(acc[c]))
  
  acc_global = float(acc_global) / float(sum(sum(m)))
  #print ('global acc = ' + str(acc_global))
  return acc, acc_global

def load_data(output_path, window_size, compute_RR_interval_feature, compute_wavelets, binary_problem):
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

  if binary_problem: 
    # Uses only two classes 
    #   [0]: N (0)
    #   [1]: SVEB, VEB, F, Q (1, 2, 3, 4)
    for i in range(0, len(train_labels), 1):
      if train_labels[i] > 0:
        train_labels[i] = 1

    for i in range(0, len(eval_labels), 1):
      if eval_labels[i] > 0:
        eval_labels[i] = 1        

  return (train_data, train_labels, eval_data, eval_labels)

# normalize data features: wave & RR intervals...
def normalize_data(train_data, eval_data):
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
  return (train_data, eval_data)


def my_model_fn(features, targets, mode, params):
  """Model function for Estimator."""

  targets_onehot = tf.one_hot(indices = targets, depth=params["num_classes"], on_value = 1)
  # Connect the first hidden layer to input layer
  # (features) with relu activation
  #first_hidden_layer = tf.contrib.layers.relu(features, 10)
  first_hidden_layer = tf.contrib.layers.fully_connected(features, params["h1"])
  # tf.nn.conv1d
  second_hidden_layer = tf.contrib.layers.fully_connected(first_hidden_layer, params["h2"])
  third_hidden_layer = tf.contrib.layers.relu(second_hidden_layer, params["h3"])

  # Connect the output layer to second hidden layer (no activation fn)
  output_layer = tf.contrib.layers.linear(third_hidden_layer, params["num_classes"])

  if mode == 'train' and params["weight_imbalanced"]:
    weights_tf = tf.constant(params["weights"])

  else:
    weights_tf = tf.ones([features.shape[0].value], tf.float32) 
      
  loss = tf.losses.softmax_cross_entropy(targets_onehot, output_layer, weights=weights_tf)
  train_op = tf.contrib.layers.optimize_loss(
    loss=loss,
    global_step=tf.contrib.framework.get_global_step(),
    learning_rate=params["learning_rate"],
    optimizer="SGD")

  correct_prediction = tf.equal(tf.argmax(targets_onehot, 1), tf.argmax(output_layer, 1))
  accuracy = tf.reduce_mean(tf.cast(correct_prediction, tf.float32))   
  eval_metric_ops = {
    "accuracy": accuracy
        #tf.metrics.accuracy(targets_onehot, output_layer)
        #"rmse": tf.metrics.root_mean_squared_error(
        #    tf.cast(targets, tf.float64), predictions)
  }

  return model_fn_lib.ModelFnOps(
    mode=mode,
    predictions=output_layer,#predictions_dict,
    loss=loss,
    train_op=train_op,
    eval_metric_ops=eval_metric_ops)


def main():
  window_size = 160
  compute_RR_interval_feature = True
  compute_wavelets = True
  dataset = '/home/mondejar/dataset/ECG/mitdb/'
  output_path = dataset + 'm_learning/'

  binary_problem = False
  weight_imbalanced = True


  # 0 Load Data
  train_data, train_labels, eval_data, eval_labels = load_data(output_path, window_size, compute_RR_interval_feature, compute_wavelets, binary_problem)

  # 1 TODO Preprocess data? norm? if RR interval, last 4 features are pre, post, local and global RR
  # Apply some norm? convolution? another approach?
  normalize = False
  if normalize:
    train_data, eval_data =  normalize_data(train_data, eval_data)
    
  # 2 Create my own model
  # Imbalanced class: weights
  # https://www.tensorflow.org/api_guides/python/contrib.losses

  # Learning rate for the model
  LEARNING_RATE = 0.001
  if binary_problem:
    num_classes = 2
  else:
    num_classes = 5
  # Set model params

  count = collections.Counter(train_labels)
  total = 0
  max_class = 0
  for c in range(0,num_classes):
    total = count[c] + total
    if count[c] > max_class:
      max_class = count[c]

  class_weight = np.zeros(num_classes)
  for c in range(0,num_classes):
    if count[c] > 0:
      #class_weight[c] = 1- float(count[c]) / float(total)
      class_weight[c] = float(max_class) / float(count[c]) # the class with more instance will have weight = 1, and the others x times ...

  # TODO give more weigth to anomaly classes? We want to detect always these bad anomalies
  weights = np.zeros((len(train_labels)), dtype='float')
  for i in range(0,len(train_labels)):
    weights[i] = class_weight[train_labels[i]]

  hn_1 = [128, 64, 32]
  hn_2 = [64, 32, 16]
  hn_3 = [32, 16, 8]
  steps = [500, 1000, 2000, 8000]
 

  for h1 in hn_1:
    for h2 in hn_2:
      for h3 in hn_3:
        for s in steps:
          model_params = {
            "learning_rate": LEARNING_RATE, 
            "num_classes": num_classes, 
            "weights": weights, 
            "weight_imbalanced": weight_imbalanced, 
            "h1": h1,
            "h2": h2, 
            "h3": h3}

          nn = tf.contrib.learn.Estimator(model_fn=my_model_fn, params=model_params)
          
          def get_train_inputs():
            x = tf.constant(train_data)
            y = tf.constant(train_labels)
            return x, y
          
          # Fit
          nn.fit(input_fn=get_train_inputs, steps=s)

          # Score accuracy
          def get_test_inputs():
            x = tf.constant(eval_data)
            y = tf.constant(eval_labels)
            return x, y
          
          ev = nn.evaluate(input_fn=get_test_inputs, steps=1)["accuracy"]

          # Compute the matrix confussion
          predictions = list(nn.predict(input_fn=get_test_inputs))

          confusion_matrix = np.zeros((num_classes,num_classes), dtype='int')
          for p in range(0, len(predictions), 1):
            ind_p = np.argmax(predictions[p])
            confusion_matrix[ind_p][eval_labels[p]] = confusion_matrix[ind_p][eval_labels[p]] + 1

          acc, acc_g = compute_accuracy(confusion_matrix)
          np.savetxt('nn_' + str(h1) + '_' + str(h2) + '_' + str(h3) + '_' + str(s) + '_cm.txt', confusion_matrix, fmt='%-7.0f')    
          np.savetxt('nn_' + str(h1) + '_' + str(h2) + '_' + str(h3) + '_' + str(s) + '_acc.txt', acc, fmt='%-7.2f')    


if __name__ == "__main__":
  main()
