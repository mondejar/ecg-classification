% Extract beats from mitdb dataset with size =  2 * window_size
%
% Author: Mondejar Guerra
% VARPA
% University of A Coruña
% April 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [dataset] = prepare_data_incartdb(window_l, window_t, compute_RR_interval_feature, display_R_peaks)
% Example callings:
% preparee_data_incartdb(90, 90, true, false)

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
selected_R{length(records)} = [];
temporal_features{length(records)} = [];
% window_size samples before and after R peak
list_classes = {'N', 'L', 'R', 'e', 'j', 'A', 'a', 'J', 'S', 'V', 'E', 'F', 'P', '/', 'f', 'u'};

for r = 1:length(records)
    r
    filename = records(r);
    full_signals = csvread(filename{1}, jump_lines);    % ommit the first line then read .csv files
    
    % This dataset contains 12 samples for the same record
    signal2_orig = full_signals(:, 3); % Modified Lead II
    signal2 = resample(signal2_orig, 360, 257); % Re-sample data

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
    
    for i = 1:length(annotations)
        if((findstr(annotations{i}, ':') > 0))
            pos_orig = str2num(annotations{i+1});
            pos = round(pos_orig * (360/257));%Re-sample this pose
            
            peak_type = 0;
            
            if(sum(strcmp(annotations{i+2}, list_classes)))
                if(pos > window_l && pos < (length(signal2) - window_t))
                    signals{r} = [signals{r} signal2(pos-window_l:pos+ window_t)];
                    classes{r} = [classes{r} annotations(i+2)];
                    selected_R{r} = [selected_R{r} 1];
                else
                    selected_R{r} = [selected_R{r} 0];
                end
            else
                selected_R{r} = [selected_R{r} 0];
            end
            
            R_poses{r} = [R_poses{r} pos];
        end
    end    
    
    
    %% Display signal and R-peaks
    if(display_R_peaks)
        plot(data2);
        hold on;
        scatter(R_poses{r}, data2(R_poses{r}),'r');
    end
    
    %% TODO Read QRS onset, offset, P, T? from ecgpuwave
    % ...
    % poses = []
    % for i = 1:length(annotations)
    %   if((findstr(annotations{i}, ':') > 0))
    %       pos = str2num(annotations{i+1});
    %       poses = [poses pos];
    %   end
    % end
    
    %% IMPORTANTE: para calcular RR se tienen que tener en cuenta todos los R peaks, si usamos solo los anotados esta mal!
    % Compute RR interval features at patients level!
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
db.temporal_features = temporal_features;
db.window_l = window_l;
db.window_t = window_t;

save([output_path, 'data_w_', num2str(window_l), '_', num2str(window_t)], 'db');
end