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
import collections
from tensorflow.contrib.learn.python.learn.estimators import model_fn as model_fn_lib


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


# normalize data features: wave & RR intervals...
def normalize_data():
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


def my_model_fn(features, targets, mode, params):
  """Model function for Estimator."""

  targets_onehot = tf.one_hot(indices = targets, depth=params["num_classes"], on_value = 1)
  # Connect the first hidden layer to input layer
  # (features) with relu activation
  #first_hidden_layer = tf.contrib.layers.relu(features, 10)
  first_hidden_layer = tf.contrib.layers.fully_connected(features, 64)

  # Connect the second hidden layer to first hidden layer with relu
  second_hidden_layer = tf.contrib.layers.fully_connected(first_hidden_layer, 128)

  third_hidden_layer = tf.contrib.layers.fully_connected(second_hidden_layer, 32)

  # Connect the output layer to second hidden layer (no activation fn)
  output_layer = tf.contrib.layers.linear(third_hidden_layer, params["num_classes"])# num classes 4

  # Reshape output layer to 1-dim Tensor to return predictions
  #predictions = tf.reshape(output_layer, [-1]) 
  #predictions_dict = {"ages": predictions}

  #TODO add the weights depending on labels 

  # Calculate loss using mean squared error
  # predictions [batch_size, params["num_classes"]]

  if mode == 'train':
    weights_tf = tf.constant(params["weights"])
  else:
    weights_tf = tf.ones([features.shape[0].value], tf.float32) 
      
  loss = tf.losses.softmax_cross_entropy(targets_onehot, output_layer, weights=weights_tf)
  # tf.losses.softmax_cross_entropy
  # tf.contrib.losses.softmax_cross_entropy
  train_op = tf.contrib.layers.optimize_loss(
    loss=loss,
    global_step=tf.contrib.framework.get_global_step(),
    learning_rate=params["learning_rate"],
    optimizer="SGD")

  # loss = tf.losses.mean_squared_error(targets, predictions)

  # Adagrad optimizer.
  # Calculate root mean squared error as additional eval metric
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

  # 0 Load Data
  train_data, train_labels, eval_data, eval_labels = load_data(output_path, window_size, compute_RR_interval_feature, compute_wavelets)

  # 1 TODO Preprocess data? norm? if RR interval, last 4 features are pre, post, local and global RR
  # Apply some norm? convolution? another approach?
  normalize = False
  #if normalize:
  # normalize_data()
    
  # [34,38] RR interval

  # 2 Create my own model

  # Specify that all features have real-value data
  #feature_columns = [tf.contrib.layers.real_valued_column("", dimension=len(train_data[0]))]

  # Build 3 layer DNN with 10, 20, 10 units respectively.

  #TODO modify the optimization parameter it must depends on accuracy taking the average accuracy 
  # of each class not only at instance level because the test contains much unbalanced between normal
  # and anomalies class

  # We can add weight on each class analyzing the training. Then:
  # we can obtain the proportions of each class in training dataset and add a factor in order to 
  # make each class matter the same

  # TODO but this must be done with my own Model Classifier at the loss selection
  #  ..tf.contrib.losses.softmax_cross_entropy(logits, onehot_labels, weight=weight)

  # https://www.tensorflow.org/api_guides/python/contrib.losses
  # ...
  # inputs, labels = LoadData(batch_size=3)
  # logits = MyModelPredictions(inputs)
  # Ensures that the loss for examples whose ground truth class is `3` is 5x
  # higher than the loss for all other examples.
  # weight = tf.multiply(4, tf.cast(tf.equal(labels, 3), tf.float32)) + 1
  # onehot_labels = tf.one_hot(labels, num_classes=5)
  # tf.contrib.losses.softmax_cross_entropy(logits, onehot_labels, weight=weight)

  # Learning rate for the model
  LEARNING_RATE = 0.001
  num_classes = 5
  # Set model params

  count = collections.Counter(train_labels)
  total = 0
  for c in range(0,num_classes):
    total = count[c] + total

  class_weight = np.zeros(5)
  for c in range(0,num_classes):
    if count[c] > 0:
      class_weight[c] = 1- float(count[c]) / float(total)

  weights = np.zeros((len(train_labels)), dtype='float')
  for i in range(0,len(train_labels)):
    weights[i] = class_weight[train_labels[i]]

  #weights = tf.multiply(4.0, tf.cast(tf.equal(tf.constant(train_labels), 3), tf.float32)) + 1
  #TODO weights = tf.mul(train_labels, class_weight) 
  model_params = {"learning_rate": LEARNING_RATE, "num_classes": num_classes, "weights": weights}

  nn = tf.contrib.learn.Estimator(model_fn=my_model_fn, params=model_params)
  def get_train_inputs():
    x = tf.constant(train_data)
    y = tf.constant(train_labels)
    return x, y
  
  # Fit
  nn.fit(input_fn=get_train_inputs, steps=2000)

  # Score accuracy
  def get_test_inputs():
    x = tf.constant(eval_data)
    y = tf.constant(eval_labels)
    return x, y
  
  ev = nn.evaluate(input_fn=get_test_inputs, steps=1)["accuracy"]
  # print("Accuracy = %s " + ev["accuracy"])

  # print("Loss: %s" % ev["loss"])
  # print("Root Mean Squared Error: %s" % ev["rmse"])

  # Print out predictions
  # predictions = nn.predict(x=prediction_set.data, as_iterable=True)
  # for i, p in enumerate(predictions):
  # print("Prediction %s: %s" % (i + 1, p["ages"]))

  # Compute the matrix confussion
  predictions = list(nn.predict(input_fn=get_test_inputs))

  confusion_matrix = np.zeros((5,5), dtype='int')
  for p in range(0, len(predictions), 1):
    #confusion_matrix[predictions[p]][eval_labels[p]] = confusion_matrix[predictions[p]][eval_labels[p]] + 1
    ind_p = np.argmax(predictions[p])
    confusion_matrix[ind_p][eval_labels[p]] = confusion_matrix[ind_p][eval_labels[p]] + 1

  print confusion_matrix

if __name__ == "__main__":
  main()
