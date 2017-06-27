%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Train & validate the LDA model.
% Maybe use crossvalidation with the train data
%
% Input:
% - window_r_beat: size of signal around each beat (center at R)
% - use_temporal_features: Add RR interval to the feature vector
% - use_morphological_features: Add morphological (wavelet, raw signal)
%                               to the feature vector
% - use_wavelets: use wavelets or raw signal as morphological features
% - norm_features: normalize each feature at beat level (individually) [0,1]
%
% Author: Mondejar Guerra
% VARPA
% University of A Coruña
% April 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function train_LDA_mitdb_Rahhal(window_r_beat, use_wavelets, use_RR_interval_features, norm_features, norm_R, DiscrimType)

% train_LDA_mitdb_Rahhal(160, true, true, true, true, 'Linear')
% train_LDA_mitdb_Rahhal(160, true, true, true, true, 'Quadratic')

% fitcdiscr:
% 'DiscrimType' A case-insensitive string with the type of the discriminant analysis.
% Specify as one of 'Linear', 'PseudoLinear', 'DiagLinear', 'Quadratic', 'PseudoQuadratic' or 'DiagQuadratic'.
% 'Gamma' Parameter for regularizing the correlation matrix of predictors.
%  For linear discriminant, you can pass a scalar [0,1].
%  If you pass 0 and if the correlation matrix is singular, FITCDISCR sets
%  'Gamma' to the minimal value required for inverting the covariance matrix.
%  If you set to 1, FITCDISCR sets the discriminant type to 'DiagLinear'.
%  If you pass a value between 0 and 1, FITCDISCR sets the discriminant type to 'Linear'.
%
%  For quadratic discriminant, you can pass either 0 or 1.
%  If you pass 0 for 'Gamma' and 'Quadratic' for 'DiscrimType', and if one of the classes
%  has a singular covariance matrix, FITCDISCR errors.
%  If you set to 1, FITCDISCR sets the discriminant type to 'DiagQuadratic'.

% 'Prior' - Prior probabilities for each class.
% Specify as one of:
% A string:
%   - 'empirical' determines class probabilities from class frequencies in Y
%   - 'uniform' sets all class probabilities equal
% A vector (one scalar value for each class)
% A structure S with two fields:
%   S.ClassProbs containing a vector of class probabilities,
%   S.ClassNames containing the class names and defining the ordering of classes used for the elements of this vector.

%% Includes
% LIBSVM
addpath('/home/imagen/mondejar/libsvm-3.22/matlab')

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 0 Load Data
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

path_dataset = '/local/scratch/mondejar/ECG/dataset/';
dataset = 'mitdb';
full_path = [path_dataset, dataset, '/m_learning/'];

list_classes = {'N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u'};
list_superclasses = {'N', 'SVEB', 'VEB', 'F', 'Q'};

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
                [C, L] = wavedec(mit_data.signals{s}(:, b), 8, 'db4');
                app_w = appcoef(C, L, 'db4', 4);
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

num_classes = 5;
% Superclasses
% 1 N    {'N', 'L', 'R', 'e', 'j'}
% 2 SVEB {'A', 'a', 'J', 'S'}
% 3 VEB  {'V', 'E'}
% 4 F    {'F'}
% 5 Q    {'P', '/', 'f', 'u'};  Optional...

train_data = [];
test_data = [];

train_label = [];
test_label = [];

subclasses{1} = {'N', 'L', 'R', 'e', 'j'};
subclasses{2} = {'A', 'a', 'J', 'S'};
subclasses{3} = {'V', 'E'};
subclasses{4} = {'F'};
subclasses{5} = {'P', '/', 'f', 'u'};

%     Train    Test    Total
% N                    75014   
% L                     8071
% R                     7255
% e                       16
% j                      229
% A                     2546                    
% a                      150
% J                       83
% S                        2
% V                     7129
% E                      106
% F                      802
% P                        0
% /                     7023
% f                      982
% u                        0

max_temp_f = 1000;% definimos este valor suponiendo el max de valor temporal
for(p = 1:size(mit_data.signals,2))
    if( sum(mit_data.patients(p) == patients_train))% Train
        if(norm_R)
            train_data = [train_data [mit_data.features_wave{p}; mit_data.temporal_features{p}.pre_R/ max_temp_f; mit_data.temporal_features{p}.post_R / max_temp_f; mit_data.temporal_features{p}.local_R / max_temp_f; mit_data.temporal_features{p}.global_R/ max_temp_f]];
        else
            train_data = [train_data [mit_data.features_wave{p}; mit_data.temporal_features{p}.pre_R; mit_data.temporal_features{p}.post_R; mit_data.temporal_features{p}.local_R; mit_data.temporal_features{p}.global_R]];
        end
        
        %train_label = [train_label; [mit_data.classes{p}(:)]];
        % Super classes
        for(b=1:length(mit_data.classes{p}))
            for(n=1:num_classes)
                if( sum(strcmp(mit_data.classes{p}(b), subclasses{n})))
                    superclass =n;
                end
            end
            train_label = [train_label; list_superclasses(superclass)];
        end
        
    else% Test
        if(norm_R)
            test_data = [test_data [mit_data.features_wave{p}; mit_data.temporal_features{p}.pre_R/max_temp_f; mit_data.temporal_features{p}.post_R/max_temp_f; mit_data.temporal_features{p}.local_R/max_temp_f; mit_data.temporal_features{p}.global_R/max_temp_f]];
        else
            test_data = [test_data [mit_data.features_wave{p}; mit_data.temporal_features{p}.pre_R; mit_data.temporal_features{p}.post_R; mit_data.temporal_features{p}.local_R; mit_data.temporal_features{p}.global_R]];
        end
        %test_label = [test_label; [mit_data.classes{p}(:)]];
        % Super classes
        for(b=1:length(mit_data.classes{p}))
            for(n=1:num_classes)
                if( sum(strcmp(mit_data.classes{p}(b), subclasses{n})))
                    superclass =n;
                end
            end
            test_label = [test_label; list_superclasses(superclass)];
        end
    end
end
test_data = test_data';
train_data = train_data';

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 3 Classification SVM
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
gamma = 0.5;
model_name = [full_path,  'LDA_models/lda_Rahhal_w_', num2str(window_r_beat * 2)];

model_name = [model_name, '_', DiscrimType];

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

model_name = [model_name, '_g_', num2str(gamma), '.mat'];

if(exist(model_name) == 2)
    load(model_name);
else
    model_LDA = fitcdiscr(train_data, train_label,'DiscrimType', DiscrimType, 'Gamma', gamma);
    %% WRITE DATA
    save([model_name], 'model_LDA');
end

predicted_classes = predict(model_LDA, test_data);
% Confussion matrix...
%
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

for(i =1:length(predicted_classes))
    index_p = find(strcmp(predicted_classes(i), list_superclasses)==1);
    index_gt = find(strcmp(test_label(i), list_superclasses)==1);
    
    confussion_matrix(index_p, index_gt) = confussion_matrix(index_p, index_gt)+1;
end

confussion_matrix

total_acc = 0;
for(i = 1:num_classes)
    total_acc = total_acc + confussion_matrix(i,i);
end
total_acc = total_acc / length(predicted_classes);

total_acc

end