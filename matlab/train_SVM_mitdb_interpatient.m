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

function train_SVM_mitdb_interpatient(window_r_beat, use_wavelets, use_RR_interval_features, norm_features, norm_R, weight_imbalanced, svm_type, kernel_type)
% train_SVM_mitdb_interpatient(160, true, true, true, true, true, 0, 2)

%% Includes
% LIBSVM
addpath('/home/mondejar/libsvm-3.22/matlab')

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 0 Load Data
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

path_dataset = '/home/mondejar/dataset/ECG/';
dataset = 'mitdb';
full_path = [path_dataset, dataset, '/m_learning/'];

list_classes = {'N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u'};
for(i=1:length(list_classes))
   if(strcmp(list_classes{i}, '/'))
       list_classes{i} = '\';
   end
end

load([full_path, 'data_w_', num2str(window_r_beat * 2), '_', list_classes{:}]);

%% Norm values in 0-1
max_Amp = 2048;
min_Amp = 0;

for(p = 1:size(mit_data.signals))
    mit_data.signals{1}  = ([mit_data.signals{1}]  - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 1 Feature extraction
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if(use_wavelets )% Full Signal
    % Wavelets
    features_wave{size(mit_data.signals, 2)} = [];
    
    % Check if they have been already computed
    wavelet_features_filename = [full_path, 'wavelet_feature_w_', num2str(window_r_beat * 2), '.mat'];
    if(exist(wavelet_features_filename, 'file') ~= 2)
        
        for(s = 1:size(mit_data.signals, 2))
            for(b =1:size(mit_data.signals{s}, 2))
                [C, L] = wavedec(mit_data.signals{s}(:, b), 4, 'db8');
                app_w = appcoef(C, L, 'db8', 4);
                features_wave{s}(1:length(app_w), b) = app_w;
            end
        end
        
        mit_data.features_wave = features_wave;
        save(wavelet_features_filename, 'features_wave');
    else
        load(wavelet_features_filename);
        mit_data.features_wave = features_wave;
    end
end

if(norm_features)
    for(s=1:length(features_wave))
        for(b=1:size(features_wave{s},2))
            maximum = max(features_wave{s}(:, b));
            minimum = min(features_wave{s}(:, b));
            features_wave{s}(:, b) = (features_wave{s}(:, b) - minimum) / (maximum - minimum);
        end
    end
    mit_data.features_wave = features_wave;
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 2 Split data in Train/Test
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
n_features = size(mit_data.features_wave{1}, 1);

patients_train = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230];
patients_test = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234];

num_classes = 4;
% Superclasses
% 1 N    {'N', 'L', 'R', 'e', 'j'}
% 2 SVEB {'A', 'a', 'J', 'S'} 
% 3 VEB  {'V', 'E'}
% 4 F    {'F'}
% 5 Q    {'P', '/', 'f', 'u'};  Optional...

train_data = [];
test_data = [];

train_label{num_classes} = [];
test_label{num_classes} = [];

subclasses{1} = {'N', 'L', 'R', 'e', 'j'};
subclasses{2} = {'A', 'a', 'J', 'S'} ;
subclasses{3} = {'V', 'E'};
subclasses{4} = {'F'};
%subclasses{5} = {'P', '/', 'f', 'u'}; 

max_temp_f = 1000;% definimos este valor suponiendo el max de valor temporal
b_i = 0;
for(p = 1:size(mit_data.signals,2))
    
    if( sum(mit_data.patients(p) == patients_train))% Train
        
       if(norm_R)
            train_data = [train_data [mit_data.features_wave{p}; mit_data.temporal_features{p}.pre_R/ max_temp_f; mit_data.temporal_features{p}.post_R / max_temp_f; mit_data.temporal_features{p}.local_R / max_temp_f; mit_data.temporal_features{p}.global_R/ max_temp_f]];
       else
            train_data = [train_data [mit_data.features_wave{p}; mit_data.temporal_features{p}.pre_R; mit_data.temporal_features{p}.post_R; mit_data.temporal_features{p}.local_R; mit_data.temporal_features{p}.global_R]];
       end
       for(b=1:size(mit_data.classes{p},2))
           for(n=1:num_classes)
               if(sum(strcmp(mit_data.classes{p}(b) , subclasses{n})))
                    train_label{n} = [train_label{n} 1];  
               else
                    train_label{n} = [train_label{n} -1];        
               end   
           end
       end
       
   else% Test
               
       if(norm_R)
            test_data = [test_data [mit_data.features_wave{p}; mit_data.temporal_features{p}.pre_R/max_temp_f; mit_data.temporal_features{p}.post_R/max_temp_f; mit_data.temporal_features{p}.local_R/max_temp_f; mit_data.temporal_features{p}.global_R/max_temp_f]];
       else
            test_data = [test_data [mit_data.features_wave{p}; mit_data.temporal_features{p}.pre_R; mit_data.temporal_features{p}.post_R; mit_data.temporal_features{p}.local_R; mit_data.temporal_features{p}.global_R]];
       end
       
          
       for(b=1:size(mit_data.classes{p},2))
           b_i = b_i+1;
           g_truth_test(b_i) = 5;

           for(n=1:num_classes)
               if(sum(strcmp(mit_data.classes{p}(b) , subclasses{n})))

                   g_truth_test(b_i) = n;
                    test_label{n} = [test_label{n} 1];  
               else
                    test_label{n} = [test_label{n} -1];        
               end   
           end
       end
    end
end
test_data = test_data';
train_data = train_data';

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 3 Classification SVM
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%best_C = 32;
%best_gamma = 0.05;


best_C = 64;
best_gamma = 0.05;

model_name = [full_path,  'SVM_models/svm_interpt_w_', num2str(window_r_beat * 2)];

% if(svm_kernel == )
%    model_name = [model_name, '_'];
% end

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

if(weight_imbalanced)
    model_name = [model_name, '_wi'];
end


model_name = [model_name, '_C_', num2str(best_C), '_g_', num2str(best_gamma), '_one_vs_all.mat'];

if(exist(model_name) == 2)
    load(model_name);
else
    for(k =1:num_classes)
        if(weight_imbalanced)
            w0 =  size(train_data, 1) / (size(train_data, 1) - size(find(train_label{k} == 1), 2))
            w1 =  size(train_data, 1) / size(find(train_label{k} == 1), 2) 
            model_SVM = svmtrain(train_label{k}', train_data, ['-s 0 -t 2 -b 0 -c ' num2str(best_C) ' -g ' num2str(best_gamma) ' -w0 ' num2str(w0) ' -w1 ' num2str(w1)]); %, options...
        else% -b probability_estimates
             model_SVM = svmtrain(train_label{k}', train_data, ['-s 0 -t 2 -b 0 -c ' num2str(best_C) ' -g ' num2str(best_gamma)]); %, options...           
        end
        models_SVM{k} = model_SVM;
    end    
    %% WRITE DATA
    save([model_name], 'models_SVM');
end

% Predict test
% predict will all models and select for each test instance the class with
% highest probability

probs = [];
accuracys = [];
predicted_labels = [];

% No tener en cuenta la clase Q ...
for(k =1:num_classes)
    disp('Full test');
    [predicted_label, accuracy, prob] = svmpredict(test_label{k}', test_data, models_SVM{k});
    
    accuracys = [accuracys accuracy];
    predicted_labels = [predicted_labels predicted_label];
    probs = [probs prob];
end

% Check for each test instance the max probability
[val, predicted_class] = max(probs');

%    N V R L /
%   ____________
% N| x 
% V|   x 
% R|     x
% L|       x
% /|         x
% 

% Class N
% Class anomalies V, R, L, /  subclass_A{c}
confussion_matrix = zeros(num_classes,num_classes);

for(i =1:length(predicted_class))
    if(g_truth_test(i) ~= 5)
       confussion_matrix(predicted_class(i), g_truth_test(i)) = confussion_matrix(predicted_class(i), g_truth_test(i))+1;    

       dif_selected_class_true_class(i) = probs(i,  g_truth_test(i)) - val(i);
    end

end

total_acc = 0;
for(i = 1:num_classes)
    total_acc = total_acc + confussion_matrix(i,i);
end
total_acc = total_acc / length(predicted_class);

confussion_matrix
total_acc

% Conclusiones: en general tanto para la anomalia R, L se clasifican
% demasiadas muestras como clase normal, estudiar si el tamaño de ventana o
% la informacion temporal RR puede mejorar estos casos.

% Display Top-N Hard negatives
% Definiremos seleccion hard negative como aquellos casos en los que hay un
% FP y existe una diferencia muy grande de prob entre la clase correcta y
% la elegida

%dif_selected_class_true_class

[dif_selected_class_true_class_sorted, index] = sort(dif_selected_class_true_class,'ascend');

g_truth_test(index(1:10));

% The worst classification 
plot(test_data(index(1), :))
probs(index(1),:)
end