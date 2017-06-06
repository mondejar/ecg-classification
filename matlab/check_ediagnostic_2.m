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

function check_ediagnostic_2(window_r_beat, use_wavelets, norm_features)
% check_ediagnostic_2(200, true, true)

%% Includes
% LIBSVM
addpath('/home/imagen/mondejar/libsvm-3.22/matlab')
% Pan Tompkins (R peak detection)
addpath('/home/imagen/mondejar/Dropbox/ECG/code/third_party/Pan_Tompkins_ECG_v7');

%% Load SVM model
path_dataset = '/local/scratch/mondejar/ECG/dataset/';
dataset = 'mitdb';
full_path = [path_dataset, dataset, '/m_learning/'];
verbose = 0;

%% Read Data
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Normal

data = '/local/scratch/mondejar/ECG/dataset/ediagnostic/electros/output_txt/Subclasses/';
subclasses = dir([data, '*']);
subclasses = subclasses(3:length(subclasses));

patients{length(subclasses)} = [];
RAW_signals{length(subclasses)} = [];
beats{length(subclasses)} = [];

remove_noise = false;
family = 'db8';
N = 6;

%% Peak detection
probs = [];
label_beats = [];

for d =1:length(subclasses)
    
    filesPV = dir([data, '/', subclasses(d).name, '/*.PV']);
    filesPB = dir([data, '/', subclasses(d).name, '/*.PB']);
    filesPM = dir([data, '/', subclasses(d).name, '/*.PM']);
    filesPL = dir([data, '/', subclasses(d).name, '/*.PL']);
    
    files = [filesPV; filesPB; filesPM; filesPL];
    
    for(f = 1: length(files))
        files(f).name = [ data, subclasses(d).name, '/', files(f).name];
        patients{d} = [patients{d}; files(f)];
    end
    
    % Extract signal from files and do the baseline/noise removal...
    RAW_signals{d} = extract_and_preprocess_signal(patients{d}, N, family, false, verbose);
    
    RAW_signals_remv_noise{d} = extract_and_preprocess_signal(patients{d}, N, family, true, verbose);
    % Find beats around R-peaks 
    
    for(s=1:size(RAW_signals_remv_noise{d},2))
        %'Threshold' — Minimum height difference
        signal = RAW_signals_remv_noise{d}(:,s);
        
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
                
                %label_beats = [label_beats, labels(s)];
                beats{d} = [beats{d}, beat];
            end
        end
    end
end



% Evaluate beats from subclasses:
% 4 BLOQUEO_AVANZADO_DE_RAMA_DERECHA R
beats_R = beats{4};
labels_R = repmat(-1, size(beats_R, 2), 1);
% 5 BLOQUEO_AVANZADO_DE_RAMA_IZQUIERDA L
beats_L = beats{5};
labels_L = repmat(-1, size(beats_L, 2), 1);
% 7 NORMAL
beats_N = beats{7};
labels_N = repmat(1, size(beats_N, 2), 1);

% Many one-vs-all svms
list_anomalies = {'V', 'R', 'L', '/'};

model_name = [full_path,  'SVM_models/svm_w_', num2str(window_r_beat * 2)];

best_C = 64;
best_gamma = 0.5;

% if(svm_kernel == )
%    model_name = [model_name, '_'];
% end
use_wavelets = true;
norm_features = true;
use_RR_interval_features = false;
norm_R = false;

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

load([model_name, '_one_vs_all.mat']);


% Predict
probsL = [];
probsR = [];
probsN = [];

for(k =1:length(list_anomalies)+1)
    disp('Full test');
    [predicted_label, accuracy, prob] = svmpredict(labels_L, beats_L', models_SVM{k});
    probsL = [probsL prob];
    
    [predicted_label, accuracy, prob] = svmpredict(labels_R, beats_R', models_SVM{k});
    probsR = [probsR prob];
    
    [predicted_label, accuracy, prob] = svmpredict(labels_N, beats_N', models_SVM{k});
    probsN = [probsN prob];
end

[val, predicted_class] = max(probsL');

end