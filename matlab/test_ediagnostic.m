%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Create a dataset from ediagnostic files
% The script extract the derivation II from the samples and check (at beat
% level) if the beat is normal/anomalies. Then compute the accuracy at
% signal level (i.e if at least one beat from an anomalie signal is label
% as anomalie, that full signal is considered correct)
%
% Author: Mondejar Guerra
% VARPA
% University of A Coruña
% April 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function test_ediagnostic(window_r_beat, use_RR_interval_features, use_wavelets, norm_features, norm_R, svm_type, kernel_type)
% test_ediagnostic(200, true, true, true, true, 2, 2) % OVSVM

% SVM model
ind_model = 10;

%% Includes
% LIBSVM
addpath('/home/imagen/mondejar/libsvm-3.22/matlab')
% Pan Tompkins (R peak detection)
addpath('/home/imagen/mondejar/Dropbox/ECG/code/third_party/Pan_Tompkins_ECG_v7');

%% Load SVM model
path_dataset = '/local/scratch/mondejar/ECG/dataset/';
dataset = 'mitdb';
full_path = [path_dataset, dataset, '/m_learning/'];

model_name = [full_path,  'SVM_models/svm_w_', num2str(window_r_beat * 2)];

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

best_C = 2;
best_gamma = 2;

model_name = [model_name, '_C_', num2str(best_C), '_g_', num2str(best_gamma)];
model_name

load([model_name, '.mat'], 'model_SVM');
family = 'db8';
N = 6;
s = 0;
verbose = 0;

%% Read Data
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Normal
data_Normal = '/local/scratch/mondejar/ECG/dataset/ediagnostic/electros/output_txt/N/';
filesPV = dir([data_Normal, '*.PV']);
filesPB = dir([data_Normal, '*.PB']);
filesPM = dir([data_Normal, '*.PM']);
filesPL = dir([data_Normal, '*.PL']);

files = [filesPV; filesPB; filesPM; filesPL];
%% TODO .PV .PB .PL .PM
% PV 200HZ  el resto 300HZ

patients_N = [];
for(f = 1: length(files))
    files(f).name = [ data_Normal files(f).name];
    patients_N = [patients_N; files(f)];
end

remove_noise = false;
signals_N = extract_and_preprocess_signal(patients_N, N, family, remove_noise, verbose);  

%% Anomalies
data_Anomalies = '/local/scratch/mondejar/ECG/dataset/ediagnostic/electros/output_txt/A/';
dirs = dir([data_Anomalies, '*']);
patients_A = [];

filesPV = dir([data_Anomalies, '/*.PV']);
filesPB = dir([data_Anomalies, '/*.PB']);
filesPM = dir([data_Anomalies, '/*.PM']);
filesPL = dir([data_Anomalies, '/*.PL']);
files = [filesPV; filesPB; filesPM; filesPL];

for(f = 1: length(files))
    files(f).name = [ data_Anomalies files(f).name];
    patients_A = [patients_A; files(f)];
end

% Extract signal from files and do the baseline/noise removal...
signals_A = extract_and_preprocess_signal(patients_A, N, family, remove_noise, verbose);
       
%% Evaluation 
labels =  [ones(1, size(signals_N, 2)) -1* ones(1, size(signals_A, 2))];
signals = [signals_N signals_A];

TP = 0;
TN = 0;
FP = 0;
FN = 0;

TP_raw = 0;
TN_raw = 0;
FP_raw = 0;
FN_raw = 0;

%% Peak detection
probs = [];
beats = [];
label_beats = [];

for(s=1:size(signals,2))       
     %'Threshold' — Minimum height difference
     signal = signals(:,s);
    
     %% Detect QRS complex 
     % ecg : raw ecg vector signal 1d signal
     % fs : sampling frequency e.g. 200Hz, 400Hz and etc
     % gr : flag to plot or not plot (set it 1 to have a plot or set it zero not
     % This method compute the R points and also do noise removal to the
     % signal
     [qrs_amp_raw, qrs_i_raw, delay] = pan_tompkin(signal, 360, 0);
     
     % Salvo algunas señales raras va bien
     % Display R
     %clf;
     %subplot(1,1,1);
     %plot(signal);
     %hold on;
     %scatter(qrs_i_raw, signal(qrs_i_raw),'r');       
     if(use_RR_interval_features)
         pre_R = 0;
         post_R = qrs_i_raw(2) - qrs_i_raw(1);
         avg_9_pre_R = [];
        
         for(i=2:length(qrs_i_raw)-1)
           pre_R = [pre_R, qrs_i_raw(i) - qrs_i_raw(i-1)];            
           post_R = [post_R, qrs_i_raw(i+1) - qrs_i_raw(i)];            
         end
        pre_R(1) = pre_R(2);
        pre_R = [pre_R, qrs_i_raw(length(qrs_i_raw)) - qrs_i_raw(length(qrs_i_raw)-1)];            

        post_R = [post_R, post_R(length(qrs_i_raw)-1)];
        
        % AVG -4:+4  9
        for(i=1:length(qrs_i_raw))
           window = i-4:i+4;
           valid_window_b = window > 0; 
           valid_window_t = window < length(qrs_i_raw); 
           valid_window = valid_window_b .* valid_window_t;
           window = window .* valid_window;
           window = window(window~=0);

           avg_val = sum(pre_R(window));
           avg_val = avg_val / (sum(valid_window));
           
           avg_9_pre_R = [avg_9_pre_R, avg_val];           
        end
     end
     
    % Window??
    % For each R set a window 100 after 100 before the signal
    %% TODO convertir la frecuencia, se debe trabajar a la misma que el dataset de training
    %% o ajustar el dataset de training...
    predictions = [];
    
    for(r=1:length(qrs_i_raw)) % for each beat 
        if(qrs_i_raw(r) > window_r_beat && qrs_i_raw(r) + window_r_beat < length(signal) )
                
            beat =  signal(qrs_i_raw(r)-window_r_beat+1 : qrs_i_raw(r)+window_r_beat);
            %subplot(1,1,1);
            %plot(beat);
            
            if(use_wavelets)           
                % Create the feature vector and add to test set
                [C, L] = wavedec(beat, 8, 'db4');
                beat = appcoef(C, L, 'db4', 4);                
            end
                          
            if(norm_features)
               maximum = max(beat);
               minimum = min(beat);
               beat = (beat - minimum) / (maximum - minimum);
            end
            
            if(use_RR_interval_features)
               n_features = length(beat);
               if(norm_R)
                    pre_R(r) = pre_R(r) / 1000;
                    post_R(r) = post_R(r) / 1000;
                    avg_9_pre_R(r) = avg_9_pre_R(r) / 1000;                        
               end
               beat(1:n_features+3) = [beat(1:n_features); pre_R(r); post_R(r); avg_9_pre_R(r)];
            end
            %app_w = app_w + 1.9;
                     
            label_beats = [label_beats, labels(s)];
            beats = [beats, beat];
            %dataW_Test(1:length(app_w), s) = app_w; 
            [prediction, acc, prob] = svmpredict(labels(s), beat', model_SVM);
            predictions = [predictions, prediction];
            probs = [probs, prob];
        end
    end
    
    %% Confusion Matrix
    if(labels(s) == 1)
        if(length(predictions) == sum(predictions))
            TP = TP + 1;
        else
            TN = TN + 1;
        end
        TP_raw = TP_raw + sum(predictions == 1);
        TN_raw = TN_raw + sum(predictions == -1);
        
    elseif(labels(s) == -1)
        if(length(predictions) == sum(predictions))
            FN = FN + 1;
        else
            FP = FP + 1;
        end
        
        FP_raw = FP_raw + sum(predictions == -1);
        FN_raw = FN_raw + sum(predictions == 1);
    end
        
    % Save as test dataset/ with label to evaluate
    % The signals must be splitted but evaluate "together", if one beat
    % contains the anomaly the full signal is labeled as anormal
end

SPC = TN / (TN + FP);
SEN = TP / (TP + FN);

%fall-out or false positive rate (FPR)
FPR = 1 - SPC;

% Total de pacientes con anomalias entre el total de pacientes con
% anomalias clasificados correctamente
RATIO_F = FP / length(patients_A);
RATIO_T = TP / length(patients_N);
% Save test data
% Set the anormal samples as one matrix for each sample, then process all
% of them by signal inside a loop and compute the accuracy manually. If one
% beat of the signal is labeled as "anormal" by the model, we will
% considered the full signal OK

% No normal
% read subdirs

%% Sort beat signals by probs values from SVM
[sorted_probs, index_sort] = sort(probs, 'descend');
label_beats_sort = label_beats(index_sort);
beats_sort = beats(:,index_sort);

end