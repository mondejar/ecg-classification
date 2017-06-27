function [ output_args ] = display_mitdb( input_args )
% Plot the mitdb: two signals and the annotations

NORM_RANGE = true;
DISPLAY_IN_SEC = false;


%% First read the dataset
path_dataset = '/local/scratch/dataset/';
dataset = 'mitdb';

fs = 360;
jump_lines = 1;
low_th = fs * 5;
max_Amp = 2048;
min_Amp = 0;

num_recs = 0;
records = [];

num_annotations = 0;
annotation_files = [];

files = dir([path_dataset, dataset, '/csv']);
files([files.isdir]) = [];
for f = 1:length(files)
    filename = files(f).name;
    if( strcmp(filename(length(filename)-3:length(filename)),  '.csv') )
        num_recs = num_recs+1;
        records{num_recs} = [path_dataset, dataset, '/csv/', files(f).name];
    else
        num_annotations = num_annotations +1;
        annotation_files{num_annotations} = [path_dataset, dataset, '/csv/', files(f).name];
    end     
end   
    
%% Read data
for r = 1:length(records)
    R_peaks_annotated = [];
    state = [];
    clf; % Clean plot
    %figure;
    
    filename = records(r);
    data = csvread(filename{1}, jump_lines);    % ommit the first line then read .csv files
        
    % This dataset contains two samples for the same record
    data1 = data(:, 2); % ML-II
    data2 = data(:, 3); % V5

    % Read annotations  
    filename_annotations = annotation_files(r);
    fileID = fopen(filename_annotations{1}, 'r');
    % Skip the first line
    %       Time   Sample #  Type  Sub Chan  Num	Aux
    tline = fgets(fileID);
    
    annotations = textscan(fileID, '%s');
    annotations = annotations{1};
    for i = 1:length(annotations)
        if( (findstr(annotations{i}, ':') > 0))
            R_peaks_annotated = [R_peaks_annotated str2num(annotations{i+1})]; 
            state = [state annotations{i+2}];
            % state set the state of the patient in that beat
        end
    end      
    
    %% Preprocess
    
    %% Normalize data
    view1 = round(length(data1) / fs); % seconds
    view2 = round(length(data2) / fs); % seconds

    %% norm to range (-2.5, 2.5) V
    if(NORM_RANGE)
        data1 = (data1 - min_Amp)/ (max_Amp - min_Amp); ... % First normalize between 0,1
        data1 = (data1 * 5) -2.5; % Then change to [-2.5, 2.5]
    
        data2 = (data2 - min_Amp)/ (max_Amp - min_Amp); ... % First normalize between 0,1
        data2 = (data2 * 5) -2.5; % Then change to [-2.5, 2.5]
    end
    
    subplot(4,1,1);
    plot(data1, '-g'); 
    hold on;
    
    subplot(4,1,2);
    plot(data2, '-g'); 
    hold on;
    
    %% Display annotated points
    subplot(4,1,1);
    R_peaks_annotated = R_peaks_annotated';
    scatter(R_peaks_annotated, data1(R_peaks_annotated),'r');                 
  
    subplot(4,1,2);
    R_peaks_annotated = R_peaks_annotated';
    scatter(R_peaks_annotated, data2(R_peaks_annotated),'r');   
    pause;
end

end

