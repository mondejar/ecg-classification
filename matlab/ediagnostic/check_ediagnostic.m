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

function check_ediagnostic(window_r_beat, use_wavelets, norm_features)
% check_ediagnostic(200, true, true) 

%% Includes
% LIBSVM
addpath('/home/mondejar/libsvm-3.22/matlab')
% Pan Tompkins (R peak detection)
addpath('/home/mondejar/Dropbox/ECG/code/ecg_classification/third_party/Pan_Tompkins_ECG_v7');

%% Load SVM model
path_dataset = '/local/scratch/mondejar/ECG/dataset/';
dataset = 'mitdb';
full_path = [path_dataset, dataset, '/m_learning/'];
verbose = 0;
%% Read Data
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Normal

data_Normal = '/home/mondejar/dataset/ECG/ediagnostic/electros/output_txt/N/';
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
family = 'db8';
N = 4;
signals_N = extract_and_preprocess_signal(patients_N, N, family, remove_noise, verbose);
    
%% Anomalies
data_Anomalies = '/home/mondejar/dataset/ECG/ediagnostic/electros/output_txt/A/';
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
            
            label_beats = [label_beats, labels(s)];
            beats = [beats, beat];
        end
    end   
end

end