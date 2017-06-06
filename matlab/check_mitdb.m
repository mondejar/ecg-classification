%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Train & validate the SVM models.
% Maybe use crossvalidation with the train data
%
% Input:
% - window_r_beat: size of signal around each beat (center at R)
% - use_temporal_features: Add RR interval to the feature vector
% - use_morphological_features: Add morphological (wavelet, raw signal)
%                               to the feature vector
% - use_wavelets: use wavelets or raw signal as morphological features
% - norm_features: normalize each feature at beat level (individually) [0,1]
% - svm_type:
%       0 -- C-SVC		(multi-class classification)
%       1 -- nu-SVC		(multi-class classification)
%       2 -- one-class SVM
%       3 -- epsilon-SVR	(regression)
%       4 -- nu-SVR		(regression)
% - kernel_type:
%       0 -- linear: u'*v
%       1 -- polynomial: (gamma*u'*v + coef0)^degree
%       2 -- radial basis function: exp(-gamma*|u-v|^2)
%       3 -- sigmoid: tanh(gamma*u'*v + coef0)
%       4 -- precomputed kernel (kernel values in training_instance_matrix)
%
% Author: Mondejar Guerra
% VARPA
% University of A Coruña
% April 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function check_mitdb(window_r_beat)
% check_mitdb(200)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 0 Load Data
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

path_dataset = '/local/scratch/mondejar/ECG/dataset/';
dataset = 'mitdb';
full_path = [path_dataset, dataset, '/m_learning/'];

list_anomalies = {'V', 'R', 'L', '/'};

for(i=1:length(list_anomalies))
   if(strcmp(list_anomalies{i}, '/'))
       list_anomalies{i} = '\';
   end
end

load([full_path, 'data_w_', num2str(window_r_beat * 2), '_', list_anomalies{:}]);

%% Norm values in 0-1
max_Amp = 2047;
min_Amp = 0;

N_data = ([N_data{:}] - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
A_data = ([A_data{:}] - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1

end
