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

function train_SVM_mitdb(window_r_beat, use_RR_interval_features, use_wavelets, norm_features, norm_R, svm_type, kernel_type, test_instances)
% train_SVM_mitdb(200, true, true, true, true, 0, 2, '1') C-SVM RBF RR  1xx for test, 2xx for training
% train_SVM_mitdb(200, false, true, true, true, 0, 2, '1') C-SVM RBF 


% train_SVM_mitdb(200, true, false, true, true, 0, 2, '2') % full signal

% train_SVM_mitdb(200, false, true, true, true, 0, 2, '2')

% train_SVM_mitdb(200, true, true, true, true, 2, 2) OCSVM RBF
% train_SVM_mitdb(200, true, true, true, true,  0, 2) 
% train_SVM_mitdb(200, true, true, true, true,  2, 2) 

% train_SVM_mitdb(200, false, true, true, 0, 2) 
%% Includes
% LIBSVM
addpath('/home/imagen/mondejar/libsvm-3.22/matlab')

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
max_Amp = 2048;
min_Amp = 0;

N_data = ([N_data{:}] - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
A_data = ([A_data{:}] - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 1 Feature extraction
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if(~use_wavelets )% Full Signal
    
    features_N = N_data;
    features_A = A_data;
    
else % Wavelets
    
    % Check if they have been already computed
    wavelet_features_filename = [full_path, 'wavelet_feature_w_', num2str(window_r_beat * 2), '.mat'];
    if(exist(wavelet_features_filename, 'file') ~= 2)
        
        for(i = 1:size(N_data, 2))
            [C, L] = wavedec(N_data(:, i), 8, 'db4');
            app_w = appcoef(C, L, 'db4', 4);
            features_N(1:length(app_w), i) = app_w;
        end
        for(i = 1:size(A_data, 2))
            [C, L] = wavedec(A_data(:, i), 8, 'db4');
            app_w = appcoef(C, L, 'db4', 4);
            features_A(1:length(app_w), i) = app_w;
        end
        save(wavelet_features_filename, 'features_N', 'features_A');
    else
        load(wavelet_features_filename);
    end
end

if(norm_features)
    for(i=1:length(features_N))
        maximum = max(features_N(:, i));
        minimum = min(features_N(:, i));
        features_N(:, i) = (features_N(:, i) - minimum) / (maximum - minimum);
    end
    
    for(i=1:length(features_A))
        maximum = max(features_A(:, i));
        minimum = min(features_A(:, i));
        features_A(:, i) = (features_A(:, i) - minimum) / (maximum - minimum);
    end
end
    

if(use_RR_interval_features)
    % Add the 3 temporal Components (interval RR) to the feature vector  
    n_features = size(features_N, 1);
   
    for(i=1:length(features_N))
        pre_R = N_RR_interval.pre_R(i); 
        post_R =  N_RR_interval.post_R(i);
        avg_9_R = N_RR_interval.avg_9_pre_R(i);
        
        if(norm_R)
            pre_R = pre_R / 1000;
            post_R = post_R / 1000;
            avg_9_R = avg_9_R / 1000;                        
        end
            
        features_N(1:n_features+3, i) = [features_N(1:n_features, i); pre_R; post_R; avg_9_R];
    end
    
    for(i=1:length(features_A))
        pre_R = A_RR_interval.pre_R(i); 
        post_R =  A_RR_interval.post_R(i);
        avg_9_R = A_RR_interval.avg_9_pre_R(i);
        if(norm_R)
            pre_R = pre_R / 1000;
            post_R = post_R / 1000;
            avg_9_R = avg_9_R / 1000;                        
        end   
        features_A(1:n_features+3, i) = [features_A(1:n_features, i); pre_R; post_R; avg_9_R];
    end
    
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 2 Split data in Train/Test
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
n_features = size(features_N, 1);

% Check how many beats are from type (N, A, V..) and patient
[patients_N,~, idx] = unique(N_file);
ocurrences_N = accumarray(idx(:),1);

[patients_A,~,idx] = unique(A_file);
ocurrences_A = accumarray(idx(:),1);

%% Use the beats from patients 1xx for training and beats from patients 2xx for test
N_data_Train = [];
N_data_Test = [];
A_data_Train = [];
A_data_Test = [];

last_index = 1;
for(i = 1:length(patients_N))
     patients_N{i} =  patients_N{i}(length(patients_N{i})-6:length(patients_N{i})-4);
     
     if( patients_N{i}(1) == test_instances) % Add beat for test
        N_data_Test = [N_data_Test features_N(:, last_index:last_index + ocurrences_N(i) -1)];
        last_index = last_index + ocurrences_N(i);

     else % Add beat for training
        N_data_Train = [N_data_Train features_N(:, last_index:last_index + ocurrences_N(i) -1)];
        last_index = last_index + ocurrences_N(i);
     end
end

last_index = 1;
for(i = 1:length(patients_A))
    patients_A{i} =  patients_A{i}(length(patients_A{i})-6:length(patients_A{i})-4);
    if( patients_A{i}(1) == test_instances) % Add beat for test
        A_data_Test = [A_data_Test features_A(:, last_index:last_index + ocurrences_A(i) -1)];
        last_index = last_index + ocurrences_A(i);

    else % Add beat for training
        A_data_Train = [A_data_Train features_A(:, last_index:last_index + ocurrences_A(i) -1)];
        last_index = last_index + ocurrences_A(i);
    end
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 3 Classification SVM
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
model_count = 0;

% -c cost : set the parameter C of C-SVC
% -n nu : one-class SVM

gamma_values = [0.001, 0.01, 0.1, 1]; %[0.5/n_features, 1/n_features, 2/n_features];
if(svm_type == 2) % OCSVM
    C_values = [0.75];%0.1, 0.5]; % nu set values between [0,1]

    % Only positive class (Normal)
    train_data = N_data_Train';
    train_label = repmat(1, size(N_data_Train, 2), 1);    
else
    C_values = [60000];%, 10, 100];%[1 10 100]; 
    % the more low the C-value, the larger margin hyperplane be:
    %   more samples that could be missclasified during training (less overfit)
    train_data = [N_data_Train, A_data_Train]';
    train_label = [repmat(1, size(N_data_Train, 2), 1);  (repmat(-1, size(A_data_Train, 2), 1))]; 
end

test_data = [N_data_Test, A_data_Test]';
test_label = [repmat(1, size(N_data_Test, 2), 1);  (repmat(-1, size(A_data_Test, 2), 1))]; 

for(c = 1:length(C_values))
    for(g = 1:length(gamma_values))
        ['-s ' num2str(svm_type) ' -c ' num2str(C_values(c)) ' -g ' num2str(gamma_values(g))]

        if(svm_type == 0)
        
            model_SVM = svmtrain(train_label, train_data, ['-s 0 -t 2 -c ' num2str(C_values(c)) ' -g ' num2str(gamma_values(g))]); %, options...
        
        elseif(svm_type == 2)
            
            model_SVM = svmtrain(train_label, train_data, ['-s 2 -t 2 -n ' num2str(C_values(c)) ' -g ' num2str(gamma_values(g))]); %, options...
        
        end
        %% Analizar en profundidad las predicciones. Tasa de TN, FN, TP, FP...
        % test this configuration
        disp('Full test');
        svmpredict(test_label, test_data, model_SVM);
        
        disp('Only normal beats');
        [predicted_labelT, accuracyT, probT] = svmpredict(repmat(1, size(N_data_Test, 2), 1), N_data_Test', model_SVM);

        disp('Only anomaly beats');
        [predicted_labelF, accuracyF, probF] = svmpredict(repmat(-1, size(A_data_Test, 2), 1), A_data_Test', model_SVM);

        % Split test in classes in order to compute TP, FP, TN, FN
        model_count = model_count + 1;
        models{model_count} = model_SVM;

        predicted_labelsF{model_count} = predicted_labelF;
        accuracysF{model_count} = accuracyF;
        probsF{model_count} = probF;

        predicted_labelsT{model_count} = predicted_labelT;
        accuracysT{model_count} = accuracyT;
        probsT{model_count} = probT;
        % Validation results
    end
end
%% SVMP predict give a probability value for each prediction.
%% Check if we can set one threshold to increment the negative class...
%% When prob is < 0 the predicted label is set to -1.

%% WRITE DATA
model_name = [full_path,  'SVM_models/svm_w_', num2str(window_r_beat * 2)];

if(svm_type == 2)
    model_name = [model_name, '_OCSVM'];
end

%if(svm_kernel == )
%    model_name = [model_name, '_'];
%end

if(use_wavelets)
    model_name = [model_name, '_wvlt'];
end

if(use_RR_interval_features)
    model_name = [model_name, '_RR'];
end

if(norm_features)
    model_name = [model_name, '_N'];
end

if(norm_R)
    model_name = [model_name, '_NR'];
end

model_name = [model_name, test_instances];

model_name

save([model_name, '.mat'], 'models', 'C_values', 'gamma_values');

end
