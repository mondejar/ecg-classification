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
% Author: Mondejar Guerra
% VARPA
% University of A Coruña
% April 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [ output_args ] = train_SVM_one_against_one(window_l, window_t, morphology_type, use_RR, use_normalized_RR, weight_imbalanced, kernel_type)
% Use example:
% RR + normalized_RR + wavelets
% train_SVM_one_against_one(90, 90, 1, true, true, true, 0)

%% Includes
% LIBSVM
addpath('/home/mondejar/libsvm-3.22/matlab')

svm_type = 0; % C-SVM
norm_RR = false;
unit_vector = false;
standardization = true;
binary_problem = false;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 0. LOAD DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
dataset_name = 'mitdb';

patients_train = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230];
patients_test = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234];

[train_data, train_label, train_label_per_record] = load_dataset(dataset_name, patients_train, window_l, window_t, morphology_type, use_RR, use_normalized_RR, norm_RR, binary_problem);
[test_data, test_label, test_label_per_record] = load_dataset(dataset_name, patients_test, window_l, window_t, morphology_type, use_RR, use_normalized_RR, norm_RR, binary_problem);

train_data = train_data';
test_data = test_data';

%% Normalization of data
% Compute mean and desviation from each feature type in training and remove
if(standardization)   
    media = mean(train_data);
    st_desviation = std(train_data);
    
    for(i=1:size(train_data, 2))% substract
        train_data(:, i) = (train_data(:, i) - media(i)) / st_desviation(i);
        test_data(:, i) = (test_data(:, i) - media(i)) / st_desviation(i);
    end
end

% Perform the unit vector so the length vector == 1 to all instances
% train/test
if(unit_vector)
    for(i=1:size(train_data,1))
        train_data(i,:) = train_data(i,:)/ norm(train_data(i,:));
    end
    
    for(i=1:size(test_data,1))
        test_data(i,:) = test_data(i,:)/ norm(test_data(i,:));
    end
end

%% Display a plot based on two selected features from train/test_data
% ft_1 = 1;
% ft_2 = 3;
%
% figure
% subplot(2,1,1)
%
% colors_list = ['yellow', 'green', 'blue', 'black'];
% for (c=1:num_classes)
%     instances = find(train_label == c);
%     %scatter(train_data(instances, ft_1), train_data(instances, ft_2));
%     scatter(minA(instances), avgA(instances));
%     hold on;
% end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 3 Classification SVM
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% TODO: crossvalidation

% 0.00001, 0.0001,
list_C_values = [0.001, 0.01, 0.1, 1, 10, 20];%, 50, 100]
gamma = 0.1;

for(c=1:size(list_C_values,2))
    C = list_C_values(c);
    model_name = create_name_model(full_path, window_l, window_t, kernel_type, morphology_type, use_RR, use_normalized_RR, norm_RR, weight_imbalanced, standardization, unit_vector, binary_problem);
    
    if(exist(model_name) == 2)
        load(model_name);
        
    else
        % one vs one
        index = 0
        for(k=1:num_classes)
            for(kk= k+1:num_classes )
                
                instances_k = find(train_label == k);
                instances_kk = find(train_label == kk);
                
                ovo_train_data = [train_data(instances_k, :); train_data(instances_kk, :)];
                ovo_train_label = [ones(size(instances_k, 2),1); ones(size(instances_kk, 2),1)*-1];
                
                if(weight_imbalanced)
                    w0 =  size(ovo_train_data, 1) / (size(find(ovo_train_label == 1), 1))
                    w1 =  size(ovo_train_data, 1) / (size(find(ovo_train_label == -1), 1))
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
    end
    
    %% 4. RUN CLASSIFIER WITH TEST DATA
    % predict will all models and select for each test instance the class with
    % highest probability
    
    % No tener en cuenta la clase Q ...
    disp('Full test');
    
    index = 0;
    counter_class = zeros(size(test_data, 1), num_classes);
    result = zeros(num_classes, num_classes);
    predicted_labels = zeros(num_classes, num_classes);
    
    for(k=1:num_classes)
        for(kk= k+1:num_classes )
            index = index + 1;model_name = create_name_model(full_path, window_l, window_t, kernel_type, morphology_type, use_RR, use_normalized_RR, norm_RR, weight_imbalanced, standardization, unit_vector, binary_problem)
            [predicted_label, accuracy, prob] = svmpredict(ones(size(test_data, 1), 1), test_data, models_SVM{index}, '-q');
            
            %predicted_labels = [predicted_labels; predicted_label];
            counter_class(find(predicted_label == 1),k) = counter_class(find(predicted_label == 1),k) +1;
            counter_class(find(predicted_label == -1),kk) = counter_class(find(predicted_label == -1),kk) +1;
        end
    end
    
    for(i = 1: size(test_data, 1))
        % Check max class voting
        % if draw happen, then check the comparison between the draw
        % classes
        counter_class_r = counter_class(i,:);
        maximum = max(counter_class_r);
        selected_class = find(counter_class_r == maximum);
        if( size(selected_class, 2) > 1)
            if(result(selected_class(1), selected_class(2)) == 1)
                selected_class = selected_class(1);
            else
                selected_class = selected_class(2);
            end
        end
        output(i) = selected_class;
    end
    
    %% 5. COMPUTE PERFORMANCE MEASURES EVALUATION
    
    %% Note: Chazal, leer pag 8:
    % "The objective of this section of the study was to select a classifier configuration
    % with the best performance in separating all classes simultaneously."
    
    % To select the best configuration we trust on F-meeasure computed from
    % confussion matrix
    if binary_problem == false        
        performance_measures_per_record = evaluation_AAMI_per_record(output, test_label_per_record);
        performance_measures = evaluation_AAMI(output, test_label);     
    end
end

end



% Complete the model name based on the configuration
function model_name = create_name_model(full_path, window_l, window_t, kernel_type, morphology_type, use_RR, use_normalized_RR, norm_RR, weight_imbalanced, standardization, unit_vector, binary_problem)
   
    model_name = [full_path,  'SVM_ovo/svm_interpt_w_', num2str(window_l), '_', num2str(window_t)];
    
    if(kernel_type == 0)
        model_name = [model_name, '_lin'];
        
    elseif(kernel_type == 2)
        model_name = [model_name, '_rbf'];
    end
    
    if(morphology_type == 0)
        model_name = [model_name, '_raw'];
    end
    
    if(morphology_type == 1)
        model_name = [model_name, '_wvlt'];
    end
    
    if(morphology_type == 2)
        model_name = [model_name, '_chazl'];
    end
    
    if(morphology_type == 4)
        model_name = [model_name, '_my_morph'];
    end
    
    if(morphology_type == 5)
        model_name = [model_name, '_shape'];
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
    
    if(kernel_type == 0)
        model_name = [model_name, '_C_', num2str(C), '_one_vs_one.mat'];
    elseif(kernel_type == 2)
        model_name = [model_name, '_C_', num2str(C), '_g_', num2str(gamma), '_one_vs_one.mat'];
    end
end