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

function train_SVM_mitdb_cross_val(window_r_beat, use_RR_interval_features, use_wavelets, norm_features, norm_R)

% train_SVM_mitdb_cross_val(200, true, true, true, true)

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Cross validation
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
train_N = features_N(:, 1:15000);
test_N = features_N(:, 15001:length(features_N));

% Keep 0.5 from each class for training ? list_anomalies = {'V', 'R', 'L', '/'};
% anomaly_subclass
%train_A = features_A(:, 1:15000);
%test_A = features_A(:, 15001:length(features_A));

%Split features by class, then half by trainign and half by test
classes = unique(anomaly_subclass);

traing_test_partition = 0.5;

train_A = [];
test_A = [];
for(c =1:length(classes))
   index_sb = strcmp(anomaly_subclass, classes(c));
   index_sb = find(index_sb == 1);
   subclass_c =  features_A(:, [index_sb]);
   
   subclass_A{c} = subclass_c;
   train_A = [train_A subclass_c(:, 1:length(subclass_c)*traing_test_partition) ];
   test_A = [test_A subclass_c(:, length(subclass_c)*traing_test_partition+1:length(subclass_c))];
   
end

do_cross_val = false;
if(do_cross_val)
        
    folds = 5;
    [C,gamma] = meshgrid(-5:2:15, -15:2:3);

    %# grid search, and cross-validation
    cv_acc = zeros(numel(C),1);

    %svm_type 0
    %% With less data...
    data = [train_N, train_A]';
    labels = [repmat(1, size(train_N, 2), 1);  (repmat(-1, size(train_A, 2), 1))]; 
    % end

    % RBF C-SVM
    for i=1:numel(C)
        disp([num2str(i) ' / ' num2str(numel(C))] )
        cv_acc(i) = svmtrain(labels, data, ...
                        sprintf('-c %f -g %f -v %d', 2^C(i), 2^gamma(i), folds));
    end
    %# pair (C,gamma) with best accuracy
    [~,idx] = max(cv_acc);

    %# contour plot of paramter selection
    contour(C, gamma, reshape(cv_acc,size(C))), colorbar
    hold on
    plot(C(idx), gamma(idx), 'rx')
    text(C(idx), gamma(idx), sprintf('Acc = %.2f %%',cv_acc(idx)), ...
        'HorizontalAlign','left', 'VerticalAlign','top')
    hold off
    xlabel('log_2(C)'), ylabel('log_2(\gamma)'), title('Cross-Validation Accuracy')

    %# now you can train you model using best_C and best_gamma

    best_C = 2^C(idx);
    best_gamma = 2^gamma(idx);

    save('/home/imagen/mondejar/Dropbox/ECG/cross_validation_results_04_05.mat', 'cv_acc', 'best_C', 'best_gamma');

end


%% Then... set the train data from the full datasetand train again using only the best parameters C, gamma
% best_C = 32, best_gamma = 0.5, obtained during cross-validation with
% first 10 000 N samples and first 10 000 anomalies samples

best_C = 32;
best_gamma = 1;

data_train = [train_N, train_A]';
labels_train = [repmat(1, size(train_N, 2), 1);  (repmat(-1, size(train_A, 2), 1))]; 

data_test = [test_N, test_A]';
labels_test = [repmat(1, size(test_N, 2), 1);  (repmat(-1, size(test_A, 2), 1))]; 
    
model_SVM = svmtrain(labels_train, data_train, ['-s 0 -t 2 -c ' num2str(best_C) ' -g ' num2str(best_gamma)]); %, options...
 
%% WRITE DATA
model_name = [full_path,  'SVM_models/svm_w_', num2str(window_r_beat * 2)];

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

model_name = [model_name, '_C_', num2str(best_C), '_g_', num2str(best_gamma)];
model_name

save([model_name, '.mat'], 'model_SVM');

% Predict test
disp('Full test');
svmpredict(labels_test, data_test, model_SVM);
        
disp('Only normal beats');
[predicted_labelT, accuracyT, probT] = svmpredict(repmat(1, size(test_N, 2), 1), test_N', model_SVM);

disp('Only anomaly beats');
[predicted_labelF, accuracyF, probF] = svmpredict(repmat(-1, size(test_A, 2), 1), test_A', model_SVM);


end
