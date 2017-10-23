function [ output_args ] = display_mitdb( input_args )
% Plot the mitdb: two signals and the annotations

% Pan Tompkins (R peak detection)
addpath('/home/mondejar/Dropbox/ECG/code/ecg_classification/third_party/Pan_Tompkins_ECG_v7');

NORM_RANGE = true;
DISPLAY_IN_SEC = false;

subclasses{1} = {'N', 'L', 'R'};
subclasses{2} = {'A', 'a', 'J', 'S',  'e', 'j'};
subclasses{3} = {'V', 'E'};
subclasses{4} = {'F'};

%% First read the dataset
path_dataset = '/home/mondejar/dataset/ECG/';
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


load('../mit_test_output');


%% Read data
for r = 26:length(records)
    
    
    
    for i = 1:4
        pos_class{i} = [];
    end

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
    
    %% Detect R-peaks by PanTomkins algorithm
    [qrs_amp_raw, qrs_i_raw, delay] = pan_tompkin(data1, 360, 0);
   
    %% Preprocess
        %% FIRST MEDIAN FILTER
        % Each signal was processed with a median filter of 200-ms width to remove QRS complexes and P-waves.
        % 200ms at 360Hz = 72
        baseline = medfilt1(data1, 72); %medfilt1(db.signals{s}, 72);
        
        %% SECOND MEDIAN FILTER
        %The resulting signal was then processed with a median filter of 600 ms width to remove T-waves.
        % 600ms at 360Hz =
        baseline = medfilt1(baseline, 216); %medfilt1(baseline, 216);
        
        %% REMOVE BASELINE
        % The signal resulting from the second filter operation contained the baseline of the ECG signal, which was
        % then subtracted from the original signal to produce the baseline corrected ECG signal.
        data1 = data1 - baseline;
        
        
    %% Normalize data
    view1 = round(length(data1) / fs); % seconds
    view2 = round(length(data2) / fs); % seconds

    %% norm to range (-2.5, 2.5) V
    if(NORM_RANGE)
        data1 = (data1 - min_Amp)/ (max_Amp - min_Amp); ... % First normalize between 0,1
        data2 = (data2 - min_Amp)/ (max_Amp - min_Amp); ... % First normalize between 0,1
    end
    
    subplot(4,1,1);
    plot(data1, '-g'); 
    title('ML II') 
    hold on;

    %% Display annotated points
   
    % TODO: guardar los resultados asociados por beat/paciente/tiempo  y mostrarlos en este codigo, para poder estudiar el comportamiento ....
    % el descriptor completo tambien?? y el beat?
       
    R_peaks_annotated = R_peaks_annotated';
    %scatter(R_peaks_annotated, data1(R_peaks_annotated),'r');                 

    for i = 1: length(state)
        for j = 1:4
            if(sum(strcmp(state(i), subclasses{j})))
                pos_class{j} = [pos_class{j}, i];    
            end
        end
    end

    R_peaks_N = R_peaks_annotated(pos_class{1});
    R_peaks_S = R_peaks_annotated(pos_class{2});
    R_peaks_V = R_peaks_annotated(pos_class{3});
    R_peaks_F = R_peaks_annotated(pos_class{4});
    
    scatter(R_peaks_N, data1(R_peaks_N),'b');        % Blue
    hold on;
    scatter(R_peaks_S, data1(R_peaks_S),'r');       % Red
    hold on;
    scatter(R_peaks_V, data1(R_peaks_V),'c');       % 
    hold on;
    scatter(R_peaks_F, data1(R_peaks_F),'y');       % Yellow
 
    
    
    
    subplot(4,1,2);
    plot(data2, '-g'); 
    title('V1...V5..') 
    hold on;

    
    
    pat = str2num(records{r}(38:40));
    if sum(pat == db_test.patients)
        index = find([pat == db_test.patients] == 1);
        
        if index > 1
            o_b = size([db_test.signals{1:index-1}], 2);
            o_e = size([db_test.signals{1:index}], 2);
        else
            o_b = 0;
            o_e = size([db_test.signals{1:index}], 2);
        end
        
        good_index = [];
        bad_index = [];
        predicted_N = [];
        predicted_S = [];
        predicted_V = [];
        predicted_F = [];
      
        output = db_test.output(o_b+1: o_e);
        output_s = [];
        for i=1:length(output)
            class_gt = '';
            for j = 1:4
                if(sum(strcmp(db_test.classes{index}{i}, subclasses{j})))
                    class_gt = j;    
                end
            end
                
            if output(i) == class_gt;
                good_index = [good_index, db_test.R_poses{r}(i)];
            else
                bad_index = [bad_index, db_test.R_poses{r}(i)];
            end             
            
            if output(i) == 1
                predicted_N = [predicted_N, db_test.R_poses{r}(i)];
            elseif output(i) == 2
                predicted_S = [predicted_S, db_test.R_poses{r}(i)];
            elseif output(i) == 3
                predicted_V = [predicted_V, db_test.R_poses{r}(i)];
            elseif output(i) == 4
                predicted_F = [predicted_F, db_test.R_poses{r}(i)];
            end                         
        end

        subplot(4,1,3);
        plot(data1, '-g');
        title('MLII good results (blue) bad (red)') 

        hold on;       
        
        scatter(good_index, data1(good_index),'b');       
        hold on;
        scatter(bad_index, data1(bad_index),'r');       

        subplot(4,1,4);
        title('MLII Predictions Blue, Red, , Yellow..') 
        plot(data1, '-g');

     %   hold on;              
     %   scatter(predicted_N, data1(predicted_N),'b');       
     %   hold on;
     %   scatter(predicted_S, data1(predicted_S),'r');       
     %   hold on;
     %   scatter(predicted_V, data1(predicted_V),'c');       
        hold on;
        scatter(predicted_F, data1(predicted_F),'y');      
    
    end
    %  scatter(qrs_i_raw, data1(qrs_i_raw),'r');   
    %R_peaks_annotated = R_peaks_annotated';
    %scatter(R_peaks_annotated, data2(R_peaks_annotated),'r');   
    
    r
    pat
    pause;
end

end

% class_gt = [];
% for i = 1:size(labels,2)
%     for j = 1:4
%         if(sum(strcmp(labels(i), subclasses{j})))
%             class_gt = [class_gt, j];    
%         end
%     end
% end
