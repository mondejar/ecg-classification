%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Extract beats and annotations from incartdb dataset centered on R-peaks
% and size (window_l - window_t). Also resample data from 257Hz -> 360hz
%
% Author: Mondejar Guerra, Victor M
% VARPA
% University of A Coruña
% April 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [dataset] = prepare_data_incartdb(window_l, window_t, compute_RR_interval_feature, display_R_peaks, max_RR, remove_baseline, remove_high_freq)
% Example callings:
% prepare_data_incartdb(90, 90, true, false, false, true, false)
compute_HB_intervals = false;

%% INCART DB
%
% 257Hz
%
% Signals 12:
% 'sample #','I','II','III','AVR','AVL','AVF','V1','V2','V3','V4','V5','V6'
%
% Resample from 257Hz to 360Hz

%% 0 read the dataset
path_dataset = '/home/mondejar/dataset/ECG/';
dataset_name = 'incartdb';
fs_orig = 257;
fs = 360;
jump_lines = 1;

num_recs = 0;
records = [];
num_annotations = 0;
annotation_files = [];

files = dir([path_dataset, dataset_name, '/csv']);
files([files.isdir]) = [];
for f = 1:length(files)
    filename = files(f).name;
    if( strcmp(filename(length(filename)-3:length(filename)),  '.csv') )
        num_recs = num_recs+1;
        records{num_recs} = [path_dataset, dataset_name, '/csv/', files(f).name];
    else
        num_annotations = num_annotations +1;
        annotation_files{num_annotations} = [path_dataset, dataset_name, '/csv/', files(f).name];
    end
end

%% Read data
inst = 0;
filenames = []; %
patients = [];
classes{length(records)} = [];
signals{length(records)} = [];
R_poses{length(records)} = [];
Original_R_poses{length(records)} = [];

selected_R{length(records)} = [];
temporal_features{length(records)} = [];
% window_size samples before and after R peak
list_classes = {'N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u'};
raw_signals{2, length(records)} = [];

size_RR_max = 20;
for r = 1:length(records)
    r
    filename = records(r);
    full_signals = csvread(filename{1}, jump_lines);    % ommit the first line then read .csv files
    
    % This dataset contains 12 samples for the same record
    signal{1} = full_signals(:, 3); % Modified Lead II (L II) (Derivacion II)
    signal{1} = resample(signal{1}, 360, 257); % Re-sample data
    
    
    raw_signals{1,r} = full_signals(:, 3); % II
    raw_signals{2,r} = full_signals(:, 8); % V1   
    
    % Read annotations
    filename_annotations = annotation_files(r);
    filenames = [filenames filename];
    patients = [patients str2num(filename{1}(length(filename{1})-5:length(filename{1})-4))];
    
    fileID = fopen(filename_annotations{1}, 'r');
    % Skip the first line
    % Time   Sample #  Type  Sub Chan  Num	Aux
    tline = fgets(fileID);
    annotations = textscan(fileID, '%s');
    annotations = annotations{1};
    
    %% Filter signal
    s = 1;
    if(remove_baseline)       
        %% FIRST MEDIAN FILTER
        % Each signal was processed with a median filter of 200-ms width to remove QRS complexes and P-waves.
        % 200ms at 360Hz = 72
        baseline = medfilt1(signal{s}, 72); %medfilt1(db.signals{s}, 72);

        %% SECOND MEDIAN FILTER
        %The resulting signal was then processed with a median filter of 600 ms width to remove T-waves.
        % 600ms at 360Hz =
        baseline = medfilt1(baseline, 216); %medfilt1(baseline, 216);

        %% REMOVE BASELINE
        % The signal resulting from the second filter operation contained the baseline of the ECG signal, which was
        % then subtracted from the original signal to produce the baseline corrected ECG signal.
        signal{s} = signal{s} - baseline;
    end
        
    
    for i = 1:length(annotations)
        if((findstr(annotations{i}, ':') > 0))
            pos_orig = str2num(annotations{i+1});
            pos = round(pos_orig * (360/257));%Re-sample this pose
            
            
            
            pos = str2num(annotations{i+1});
            
            % The provided fiducial points occur at the instant of the
            % major local extremum of a QRS-complex (i.e., either the time
            % of the R-wave maximum or S-wave minimum). These fiducial
            % points were first obtained automatically and then manually cor-
            % rected on a beat-by-beat basis (Chazal)
            
            %% Set pos at the true max-peak in [-5,5] window
            unchanged_pos = pos;
            if(max_RR && (pos > size_RR_max && pos < (length(signal{1}) - size_RR_max)))
                [max_value, max_pos] = max(signal{1}(pos-size_RR_max:pos + size_RR_max));
                pos = (pos-size_RR_max) + max_pos;
            end
                       
            peak_type = 0;
            pos = pos-1;
            
            if(sum(strcmp(annotations{i+2}, list_classes)))
                if(pos > window_l && pos < (length(signal{1}) - window_t))
                    signals{1, r} = [signals{1, r} signal{1}(pos-window_l:pos+ window_t)];
            
                    classes{r} = [classes{r} annotations(i+2)];
                    selected_R{r} = [selected_R{r} 1];
                else
                    selected_R{r} = [selected_R{r} 0];
                end
            else
                selected_R{r} = [selected_R{r} 0];
            end
            
            R_poses{r} = [R_poses{r} pos];
            Original_R_poses{r} = [Original_R_poses{r}, unchanged_pos];
            
        end
    end
    
    %% Display signal and R-peaks
    %if(display_R_peaks)
    %    plot(data2);
    %    hold on;
    %    scatter(R_poses{r}, data2(R_poses{r}),'r');
    %end
    
    %% ATENTION: for compute RR intervals values we must consider all the R-peaks, not use only the labeled R-peaks!!
    
    % Compute RR interval features at patients level
    if(compute_RR_interval_feature)
        
        pre_R = 0;
        post_R = R_poses{r}(2) - R_poses{r}(1);
        local_R = []; % Average of the ten past R intervals
        global_R = []; % Average of the last 5 minutes of the signal
        
        for(i=2:length(R_poses{r})-1)
            pre_R = [pre_R, R_poses{r}(i) - R_poses{r}(i-1)];
            post_R = [post_R, R_poses{r}(i+1) - R_poses{r}(i)];
        end
        pre_R(1) = pre_R(2);
        pre_R = [pre_R, R_poses{r}(length(R_poses{r})) - R_poses{r}(length(R_poses{r})-1)];
        
        post_R = [post_R, post_R(length(R_poses{r})-1)];
        
        % Local R: AVG from past 10 RR intervals
        for(i=1:length(R_poses{r}))
            window = i-10:i;
            valid_window = window > 0;
            window = window .* valid_window;
            window = window(window~=0);
            avg_val = sum(pre_R(window));
            avg_val = avg_val / (sum(valid_window));
            
            local_R = [local_R, avg_val];
        end
        
        % Global R: AVG from past 5 minutes
        % 360 Hz  5 minutes = 108000 samples;
        for(i=1:length(R_poses{r}))
            back = -1;
            back_length = 0;
            if(R_poses{r}(i) < 108000)
                window = 1:i;
            else
                while(i+back > 0 && back_length < 108000)
                    back_length =  R_poses{r}(i) - R_poses{r}(i+back);
                    back = back -1;
                end
                window = max(1,(back+i)):i;
            end
            % Considerando distancia maxima hacia atras
            avg_val = sum(pre_R(window));
            avg_val = avg_val / length(window);
            
            global_R = [global_R, avg_val];
        end
        
        %% Only keep those features from beats that we save list_classes
        %% but for the computation of temporal features all the beats must be used
        temporal_features{r}.pre_R = pre_R(selected_R{r} == 1);
        temporal_features{r}.post_R = post_R(selected_R{r} == 1);
        temporal_features{r}.local_R = local_R(selected_R{r} == 1);
        temporal_features{r}.global_R = global_R(selected_R{r} == 1);
    end
end

%% Export as .mat files
output_path = [path_dataset, dataset_name, '/m_learning/'];

db.filenames = filenames;
db.patients = patients;
db.signals = signals;
db.classes = classes;
db.selected_R = selected_R;

db.raw_signals = raw_signals;

db.R_poses = R_poses;
db.temporal_features = temporal_features;
db.window_l = window_l;
db.window_t = window_t;

full_name = [output_path, 'data_w_', num2str(window_l), '_', num2str(window_t)];

if(max_RR)
    full_name = [full_name, '_max_RR'];
end

if(remove_baseline)
    full_name = [full_name, '_remove_baseline'];
end
if(remove_high_freq)
    full_name = [full_name, '_remove_high_freq'];
end

full_name
save(full_name, 'db');

end