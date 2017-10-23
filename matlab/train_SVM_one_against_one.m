%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Train & validate the SVM models using one-against-one, i.e for each pair
% of classes a SVM is built. Then the simplest voting strategy is used to
% predict the new data.
%
% Input:
% - window_l: begining of window around R
% - window_t: end of window around R
% - morphology_type: 0 raw signal
%                    1 use wavelets
%                    2 use chazal morphology
%                    3 use HOS
% - norm_RR: normalize RR information??
% - weight_imbalanced: weight each class on SVMs by the number instances
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
% Author: Mondejar Guerra, Victor M
% VARPA
% University of A Coruña
% April 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [ output_args ] = train_SVM_one_against_one(window_l, window_t, morphology_type, use_RR, use_normalized_RR, weight_imbalanced, kernel_type)
% Use example:
% RR + normalized_RR + wavelets
% train_SVM_one_against_one(90, 90, -1, true, true, true, true, 0)
% train_SVM_one_against_one(90, 90, [1, 3], true, true, true, true, 0)
% train_SVM_one_against_one(90, 90, [3, 4], true, true, true, 0)
% Testing only my binary descriptor
% train_SVM_one_against_one(90, 90, 4, false, false, false, true, 0)

% MLII, V5
%% Includes
% LIBSVM
addpath('/home/mondejar/libsvm-3.22/matlab')

signals_used = [1, 0]
class_division = 1; % zhang division, other value Chazal division **( Differences in e,j belong to N or S)
remove_baseline = false;
remove_high_freq = false;
standardization = true;
max_RR = false;     % load dataset with R-peak centered at maximum in [-5, 5] window...
unit_vector = false;
binary_problem = false;
use_HB = false;
do_pca = false;
svm_type = 0; % C-SVM
norm_RR = false;

num_classes = 4;
evaluate_incartdb = false;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 1. LOAD DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
dataset_name = 'mitdb';
full_path = ['/home/mondejar/dataset/ECG/',  dataset_name, '/m_learning/'];

patients_train = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230];
patients_test = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234];

[train_data, train_label, signal_p_train, db_train] = load_dataset(dataset_name, patients_train, window_l, window_t, morphology_type, use_HB, use_RR, use_normalized_RR, norm_RR, binary_problem, max_RR, remove_baseline, remove_high_freq, class_division, signals_used);
[test_data, test_label, signal_p_test, db_test] = load_dataset(dataset_name, patients_test, window_l, window_t, morphology_type, use_HB, use_RR, use_normalized_RR, norm_RR, binary_problem, max_RR, remove_baseline, remove_high_freq, class_division, signals_used);

if(evaluate_incartdb)
    [incart_data, incart_label, signal_p_incart, db_incart] = load_dataset('incartdb', [], window_l, window_t, morphology_type, use_HB, use_RR, use_normalized_RR, norm_RR, binary_problem, max_RR, remove_baseline, remove_high_freq, class_division, signals_used);
end

%% Normalization of data
% Compute mean and desviation from each feature type in training and remove
media = [];
st_desviation = [];

if(standardization)
    media = mean(train_data);
    st_desviation = std(train_data);
    
    for(i=1:size(train_data, 2))% for each feature
        train_data(:, i) = (train_data(:, i) - media(i)) / st_desviation(i);
    end
end

% Perform the unit vector so the length vector == 1 to all instances
% train/test
if(unit_vector)
    for(i=1:size(train_data,1))
        train_data(i,:) = train_data(i,:)/ norm(train_data(i,:));
    end
end

%% TEST PCA with RAW Signal, filtered? mean? Wavelets??
%% Compute PCA
% After standarized the data, compute PCA only for training, then save the
% matrix with the coefficients to apply for new test data

% data(instances, features_dimension)
% [pc,score,latent,tsquare] = princomp(data_classes{4}(182:185, :))
if do_pca
    coeff = pca(train_data);
    train_data = train_data(:, 1:181) * coeff;
    test_data = test_data(:, 1:181) * coeff;
    
    if(size(train_data, 2) > 20 ) % reduce
        % Reduce PCA to 10-th dimensionality
        %train_data = [train_data(:, 1:20), train_data(:, 182:185)];
        %test_data = [test_data(:, 1:20), test_data(:, 182:185)];
        train_data = train_data(:, 1:20);
        test_data = test_data(:, 1:20);
    end
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 2. Classification SVM
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
max_range = 1;
min_range = -1;

%% TODO: crossvalidation option...
dir_path = create_dir_path(kernel_type, morphology_type, use_HB, use_RR, use_normalized_RR, norm_RR, weight_imbalanced, standardization, unit_vector, binary_problem, max_RR, remove_baseline, remove_high_freq, class_division, do_pca, signals_used);

% 0.00001, 0.0001,
%list_C_values = [0.001, 0.01, 0.1, 1, 10, 20];%, 50, 100]
list_C_values = [0.0004];%, 0.001, 0.1, 1];%[0.01, 0.1, 0.75, 2];
gamma = 0.1;

for(c=1:size(list_C_values,2))
    disp('c value');
    C = list_C_values(c)
    model_name = create_name_model(full_path, C, window_l, window_t, kernel_type, morphology_type, use_HB, use_RR, use_normalized_RR, norm_RR, weight_imbalanced, standardization, unit_vector, binary_problem, max_RR, remove_baseline, remove_high_freq, class_division, do_pca, signals_used);
    
    %if(exist(model_name) == 2)
    %    load(model_name);
    %else
        % one vs one
        index = 0
        for(k=1:num_classes-1)
            for(kk= k+1:num_classes )
                
                instances_k = find(train_label == k);
                instances_kk = find(train_label == kk);
                
                ovo_train_data = [train_data(instances_k, :); train_data(instances_kk, :)];
                ovo_train_label = [ones(size(instances_k, 2),1); ones(size(instances_kk, 2),1)*-1];
                
                %% TRAIN WITH MIT-DB DS1
                if(weight_imbalanced)
                    w0 =  size(ovo_train_data, 1) / (size(find(ovo_train_label == 1), 1))
                    w1 =  size(ovo_train_data, 1) / (size(find(ovo_train_label == -1), 1))
                    
                    % Use w0, w1 
                    % w0 =  size(train_data, 1) / (size(find(ovo_train_label == 1), 1))
                    % w1 =  size(train_data, 1) / (size(find(ovo_train_label == -1), 1))                   
                    
                    model_SVM = svmtrain(ovo_train_label, ovo_train_data, ['-s ' num2str(svm_type) ' -t ' num2str(kernel_type) ' -b 0 -c ' num2str(C) ' -g ' num2str(gamma) ' -w1 ' num2str(w0) ' -w-1 ' num2str(w1)])%, options...
                else% -b probability_estimates
                    model_SVM = svmtrain(ovo_train_label, ovo_train_data, ['-s ' num2str(svm_type) ' -t ' num2str(kernel_type) ' -b 0 -c ' num2str(C) ' -g ' num2str(gamma)])%, options...
                end
                
                index = index + 1;
                models_SVM{index} = model_SVM;
            end
        end
        
        %% WRITE DATA
        if(standardization)
            save([model_name], 'models_SVM', 'media', 'st_desviation');
        else
            save([model_name], 'models_SVM');
        end
    %end
    
   %% TEST CONFIGURATION ON MIT-DB DS2    
   [performance_measures_DS2, output] = test_SVM_one_vs_one(num_classes, standardization, unit_vector, models_SVM, media, st_desviation, test_data, test_label, dir_path, C);
   % db_test.output = output;
   % save('mit_test_output', 'db_test');
    
    %% TEST ON INCART DB
    if(evaluate_incartdb)
        performance_measures_INCART = test_SVM_one_vs_one(num_classes, standardization, unit_vector, models_SVM, media, st_desviation, incart_data, incart_label, dir_path, C);
        % Compute Cohen's Kappa
        %[kappa_Incart, prob_expected_Incart, prob_observed_Incart] = compute_kappa(performance_measures_INCART.confussion_matrix);
    end
    
    % Write scores
    %% TODO: compute performance measure to rank the best configurations...
    %% Note: Chazal, leer pag 8:
    % "The objective of this section of the study was to select a classifier configuration
    % with the best performance in separating all classes simultaneously."
    % To select the best configuration we trust on F-meeasure computed from confussion matrix
    
end

end


% Complete the model name based on the configuration
function model_name = create_name_model(full_path, C, window_l, window_t, kernel_type, morphology_type, use_HB, use_RR, use_normalized_RR, norm_RR, weight_imbalanced, standardization, unit_vector, binary_problem, max_RR, remove_baseline, remove_high_freq, class_division, do_pca, signals_used)

model_name = [full_path,  'SVM_ovo/svm_interpt_w_', num2str(window_l), '_', num2str(window_t)];
   
if(kernel_type == 0)
    model_name = [model_name, '_lin'];
    
elseif(kernel_type == 2)
    model_name = [model_name, '_rbf'];
end


if signals_used(1) == 1
    model_name = [model_name, '_MLII'];
end
if signals_used(2) == 1
    model_name = [model_name, '_V5'];
end

if(sum(morphology_type == 0))
    model_name = [model_name, '_raw'];
end

if(sum(morphology_type == 1))
    model_name = [model_name, '_wvlt'];
end

if(sum(morphology_type == 2))
    model_name = [model_name, '_chazl'];
end

if(sum(morphology_type == 3))
    model_name = [model_name, '_HOS'];
end

if(sum(morphology_type == 4))
    model_name = [model_name, '_my_morph'];
end

if(sum(morphology_type == 5))
    model_name = [model_name, '_shape'];
end

if(sum(morphology_type == 6))
    model_name = [model_name, '_Hjorth'];
end

if (use_HB)
    model_name = [model_name, '_HB'];
end

if (use_RR)
    model_name = [model_name, '_RR'];
end

if(use_normalized_RR)
    model_name = [model_name, '_AVG_RR'];
end

if(norm_RR)
    model_name = [model_name, '_NR'];
end

if(weight_imbalanced)
    model_name = [model_name, '_wi'];
end

if(standardization)
    model_name = [model_name, '_std'];
end
if(unit_vector)
    model_name = [model_name, '_uv'];
end

if(binary_problem)
    model_name = [model_name, '_two_class'];
end

if(max_RR)
    model_name = [model_name, '_max_RR'];
end

if(remove_baseline)
    model_name = [model_name, '_remov_baseline'];
end

if(remove_high_freq)
    model_name = [model_name, '_remove_HF'];
end

if(class_division == 1)
    model_name = [model_name, '_zhang_class'];
else
    model_name = [model_name, '_chazal_class'];
end

if(do_pca)
    model_name = [model_name, '_do_pca'];
end

if(kernel_type == 0)
    model_name = [model_name, '_C_', num2str(C), '_one_vs_one.mat'];
elseif(kernel_type == 2)
    model_name = [model_name, '_C_', num2str(C), '_g_', num2str(gamma), '_one_vs_one.mat'];
end

end


function dir_path = create_dir_path(kernel_type, morphology_type, use_HB, use_RR, use_normalized_RR, norm_RR, weight_imbalanced, standardization, unit_vector, binary_problem, max_RR, remove_baseline, remove_high_freq, class_division, do_pca, signals_used)

dir_path = ['results/SVM_one-vs-one'];

if(weight_imbalanced)
    dir_path = [dir_path, '/wi'];
end

if(kernel_type == 0)
    dir_path = [dir_path, '/lin'];
    
elseif(kernel_type == 2)
    dir_path = [dir_path, '/rbf'];
end

if signals_used(1) == 1
    dir_path = [dir_path, '/MLII'];
end
if signals_used(2) == 1
    dir_path = [dir_path, '/V5'];
end

if(sum(morphology_type == 0))
    dir_path = [dir_path, '/raw'];
end

if(sum(morphology_type == 1))
    dir_path = [dir_path, '/wvlt'];
end

if(sum(morphology_type == 2))
    dir_path = [dir_path, '/chazl'];
end

if(sum(morphology_type == 3))
    dir_path = [dir_path, '/HOS'];
end

if(sum(morphology_type == 4))
    dir_path = [dir_path, '/my_morph'];
end

if(sum(morphology_type == 5))
    dir_path = [dir_path, '/shape'];
end

if(sum(morphology_type == 6))
    dir_path = [dir_path, '/Hjorth'];
end

if (use_HB)
    dir_path = [dir_path, '/HB'];
end

if (use_RR)
    dir_path = [dir_path, '/RR'];
end

if(use_normalized_RR)
    dir_path = [dir_path, '/AVG_RR'];
end

if(norm_RR)
    dir_path = [dir_path, '/NR'];
end

if(standardization)
    dir_path = [dir_path, '/std'];
end
if(unit_vector)
    dir_path = [dir_path, '/uv'];
end

if(binary_problem)
    dir_path = [dir_path, '/two_class'];
end

if(max_RR)
    dir_path = [dir_path, '/max_RR'];
end 

if(remove_baseline)
   dir_path = [dir_path, '/remove_baseline'];
end

if(remove_high_freq)
   dir_path = [dir_path, '/remove_high_freq'];
end

if(class_division == 1)
    dir_path = [dir_path, '/zhang_class'];
else
   dir_path = [dir_path, '/chazal_class'];
end

if(do_pca)
    dir_path = [dir_path, '/do_pca'];
end

if exist(dir_path) == 0
    mkdir(dir_path);
end


dir_path = [dir_path, '/'];

end