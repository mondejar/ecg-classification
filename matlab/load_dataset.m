%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Load dataset and set as matrix with data and label vector:
% features selection
% select patients (optionally)
%
% Author: Mondejar Guerra, Victor M
% VARPA
% University of A Coruña
% 26 June 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% .hea
% https://physionet.org/physiotools/wag/header-5.htm

% number of signals   freq    samples
%   Gain ADC units per millivol (200)
%   11-bit resolution
%   1024 an offset such that its output was ADC units given an input exactly in the middle of its range
%   -3609 and -27349 checksum!

% 2 360 650000
% 212 200 11 1024 960 -3609 0 MLII
% 212 200 11 1024 776 -27349 0 V1

function [data, label, signal_patients, db, index_per_record, selected_beat] = load_dataset(dataset_name,patient_list, window_l, window_r, morphology_type, use_HB, use_RR, use_normalized_RR, norm_RR, max_RR, remove_baseline, remove_high_freq, class_division, signals_used, align_R, use_ecgpuwave, use_construe)
% Example of use
% patients_train = [101, 106, 108, 109, 112, 114, 115, 116, 118, 119, 122, 124, 201, 203, 205, 207, 208, 209, 215, 220, 223, 230];
% patients_test = [100, 103, 105, 111, 113, 117, 121, 123, 200, 202, 210, 212, 213, 214, 219, 221, 222, 228, 231, 232, 233, 234];
% load_dataset('mitdb', patients_train, 90, 90, 3, true, false, false, false, true, true)

path_dataset = '/home/mondejar/dataset/ECG/';
full_path = [path_dataset, dataset_name, '/m_learning/'];
%full_path = ['/home/mondejar/Dropbox/ECG/code/'];

if window_l == 256
    full_name_dataset = [full_path, 'data_'];
else
    full_name_dataset = [full_path, 'data_w_', num2str(window_l), '_', num2str(window_r)];
end

if(max_RR == 1)
    full_name_dataset = [full_name_dataset, '_max_RR'];
elseif max_RR == 2
    full_name_dataset = [full_name_dataset, '_min_RR'];
end


if(remove_baseline)
    full_name_dataset = [full_name_dataset, '_remove_baseline'];
end
if(remove_high_freq)
    full_name_dataset = [full_name_dataset, '_remove_high_freq'];
end

if use_ecgpuwave
    full_name_dataset = [full_name_dataset, '_ecgpuwave'];
end

if use_construe
    full_name_dataset = [full_name_dataset, '_construe'];
end


disp(['loading', full_name_dataset]);
load(full_name_dataset);

signal_patients = [];
%signals_per_class{4} = [];


if(isempty(patient_list) == false)
    datasetAux.patients = [];
    subclasses{1} = {''};
    for p=1:size(patient_list,2)
        pos = find(db.patients == patient_list(p));
        
        signal_patients(p) = size(db.signals{pos}, 2);

        datasetAux.patients = [datasetAux.patients patient_list(p)];
        datasetAux.signals{1, p} = db.signals{1, pos};
        datasetAux.signals{2, p} = db.signals{2, pos};
        
        datasetAux.classes{p} = db.classes{pos};
        datasetAux.temporal_features{p} = db.temporal_features{pos};
        
        if(use_HB)
            datasetAux.HB_intervals{p} = db.HB_intervals{pos};
            datasetAux.fiducial_points{p} = db.fiducial_points{pos};
        else
            datasetAux.selected_R{p} = db.selected_R{pos};
        end  
    end
    
    datasetAux.window_l = window_l;        subclasses{1} = {''};
    datasetAux.window_r = window_r;
    
    db = datasetAux;
end

if( strcmp(dataset_name, 'mitdb'))
    
    % Range of normalization depends on the info from .hea files!
    
    %% Norm values in 0-1
    
    max_Amp = 1024; % 2048 == [-1024, +1024]
    min_Amp = -1024;
    
    for(p = 1:size(db.signals, 2))
        db.signals{1, p}  = ([db.signals{1, p}]  - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
        db.signals{2, p}  = ([db.signals{2, p}]  - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
    end
    
elseif( strcmp(dataset_name, 'incartdb'))
    %% Norm values in 0-1
    max_Amp = 4000;
    min_Amp = -4000;        subclasses{1} = {''};
    
    for(p = 1:size(db.signals, 2))
        
        %% TODO, revise that! how many signals we want to use with INCARTDB
        db.signals{p}  = ([db.signals{p}]  - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
        
    end
    
end

%% align R

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 1 Feature extraction (Create descriptor)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
db.features_wave = [];

%% no morphology
if (morphology_type == -1 )
    features_wave{size(db.signals, 2)} = [];
    db.features_wave = features_wave;
end

%% RAW Signal
if(sum(morphology_type == 0 ))
    features_wave = [];
    signals1 = [];
    signals2 = []
    if signals_used(1) == 1
        signals1 = db.signals(1,:);
        signals1 = [signals1{:}];
    end
    
    if signals_used(2) == 1
        signals2 = db.signals(2,:);
        signals2 = [signals2{:}];
    end
    
    features_wave = [signals1; signals2];
    db.features_wave = features_wave;
    
end

%% Wave Signal
if(sum(morphology_type == 1 ))
    % Wavelets
    features_wave{size(db.signals, 2)} = [];
    
    for(p = 1:size(db.signals, 2))
        for(b =1:size(db.signals{1, p}, 2))
            f_wave = [];
            for s = 1:2
                if signals_used(s)
                    
                    %Daubechies
                    % Para size 181 usar un N en wavedec <= 7 
                    % fix(log2(length(beat))) = 7
                    [C, L] = wavedec(db.signals{s, p}(:, b), 3, 'db1');
                    app_w = appcoef(C, L, 'db1', 3);
                    
                    % [C, L] = wavedec(db.signals{s, p}(:, b), 2, 'db1');
                    % app_w = appcoef(C, L, 'db1', 2);
                    f_wave = [f_wave; app_w];
                    % Center app_w on its max?????
                    
                end
            end
            features_wave{p}(1:length(f_wave), b) = f_wave;
        end
    end
    
    db.features_wave = features_wave;
    
end


%% HOS
if(sum(morphology_type == 3))
    db_features = db.features_wave ;
    
    % TODO transform signal to have zero-mean
    
    % Compute the 2, 3 and 4 order statistics
    
    % 30 lag
    %lag = round(window_l/ 3); %30; % size of window for cumulants?¿
        
    n_intervls = 6;
    lag = round( (window_l + window_r )/ n_intervls);
    
    flag = 0; % can take also the value = 1
    features_wave = [];
    % center the signal with the window of size = lag to compute each statistics
    for(p = 1:size(db.signals, 2))
        for(b =1:size(db.signals{1, p}, 2))
            
            f_wave = [];
            for s=1:2
                if signals_used(s)
                    
                    %features = zeros(5,3);
                    features = zeros(n_intervls-1,2);
                    for(n = 1:n_intervls - 1) % 5
                        % beat = db.signals{s}(:,b);
                        pose = lag * n;
                        interval = db.signals{s, p}((pose - lag/2):( pose + lag/2),b);
                        %pose = 1 + (lag * (n-1));
                        %interval = db.signals{s, p}(pose:((pose-1) + lag),b);              
                        % 2 th
                        %features(n,1) = var(interval);
                        %if isnan(features(n,1))
                        %    features(n,1) = 0.0;
                        %end
                        % 3 th
                        features(n,1) = skewness(interval);
                        %features(n,2) = skewness(interval);
                        if isnan(features(n,1))
                            features(n,1) = 0.0;
                        %    features(n,2) = 0.0;
                        end
                        % 4-th
                        features(n,2) = kurtosis(interval);
                        %features(n,3) = kurtosis(interval);
                        if isnan(features(n,2))
                            features(n,2) = 0.0;
                        %    features(n,3) = 0.0;
                        end
                    end
                    f_wave = [f_wave; features];
                end
            end
            
            features_wave{p}(1:length(f_wave(:)), b) = f_wave(:);
            
        end
        
        if isempty(db_features)
            db.features_wave{p} = features_wave{p};
        else
            db.features_wave{p} = [db_features{p}; features_wave{p}];
        end
    end
    
end



%% MULTIHOS
if(sum(morphology_type == 6))
    db_features = db.features_wave ;

    %% NEW HOS With 3 scales!!!
    n_intervls = [6];%[6]
    
    for ni=1:size(n_intervls, 2)
        lag = round( (window_l + window_r )/ n_intervls(ni));

        flag = 0; % can take also the value = 1
        features_wave = [];
        % center the signal with the window of size = lag to compute each statistics
        for(p = 1:size(db.signals, 2))
            for(b =1:size(db.signals{1, p}, 2))

                f_wave = [];
                for s=1:2
                    if signals_used(s)

                        %features = zeros(5,3);
                        features = zeros(n_intervls(ni)-1,2);
                        
                        pose = 0;
                        for n = 1:(n_intervls(ni) - 1) % 5
                            % beat = db.signals{s}(:,b);
                            pose = lag * n; % pose + (lag / 2);
                            interval = db.signals{s, p}((pose - lag/2):( pose + lag/2),b); % ( 1+ (pose - lag/2):( pose + lag/2)-1,b);
                            %pose = 1 + (lag * (n-1));
                            %interval = db.signals{s, p}(pose:((pose-1) + lag),b);              
                            % 2 th
                            %features(n,1) = var(interval);
                            %if isnan(features(n,1))
                            %    features(n,1) = 0.0;
                            %end
                            % 3 th
                            features(n,1) = skewness(interval);
                            %features(n,2) = skewness(interval);
                            if isnan(features(n,1))
                                features(n,1) = 0.0;
                            %    features(n,2) = 0.0;
                            end
                            % 4-th
                            features(n,2) = kurtosis(interval);
                            %features(n,3) = kurtosis(interval);
                            if isnan(features(n,2))
                                features(n,2) = 0.0;
                            %    features(n,3) = 0.0;
                            end
                        end
                        f_wave = [f_wave; features];
                    end
                end

                features_wave{p}(1:length(f_wave(:)), b) = f_wave(:);

            end

            if isempty(db_features)
                db.features_wave{p} = features_wave{p};
            else
                db.features_wave{p} = [db_features{p}; features_wave{p}];
            end
        end
    end

end

%% my morphology
if(sum(morphology_type == 4))
    % Distance from R-peak to most prominents fiducial points
    % Adapta for V1-V5 ??? R is minimum then...
    db_features = db.features_wave;
    features_wave = [];
    
        
        %% NOTE:
        % Usar mas puntos en comparaciones? algun criterio basado en
        % distribuciones, en general.. poder definir este descriptor de una
        % manera matematica que quede mas vendible...y demostrar realmente su
        % utilidad frente a no usarlo! o usar wavelets propuestos por otros
        % trabajos.. es lo que me queda para vender como principal novedad en
        % un posible paper (lo otro que si usar fusion (similar a otros
        % trabajos) o incluso el oversampling, pueden quedar como pequeñas
        % "novedades"...
    
    
        % Realmente deberia basarme en las reglas que usen los medicos... o al
        % menos usar como atributos la informacion que usen los medicos para
        % diagnosticar una anomalia o no... ancho QRS, intervalo RR, presencia
        % de P, distancia de PQ, ...
    
        for(p = 1:size(db.signals, 2))
            for(b =1:size(db.signals{1, p}, 2))
                f_wave = [];
                for s=1:2
                    if signals_used(s)
    
                        R_pos = 91;
                        
                        % clean high freq noise
                        features = zeros(4, 1);
                        R_value = db.signals{s, p}(91, b);
    
                        x_values = zeros(4, 1);
                        y_values = zeros(4, 1);
    
                        % Obtain values and pos from the intervals
                        if s == 1
                            [y_values(1), x_values(1)] = max(db.signals{s, p}(1:40, b));
                            [y_values(2), x_values(2)] = min(db.signals{s, p}(75:85, b));
                            [y_values(3), x_values(3)] = min(db.signals{s, p}(95:105, b));
                            [y_values(4), x_values(4)] = max(db.signals{s, p}(150:181, b));
                        elseif s == 2
                            [y_values(1), x_values(1)] = min(db.signals{s, p}(1:40, b));
                            [y_values(2), x_values(2)] = max(db.signals{s, p}(75:85, b));
                            [y_values(3), x_values(3)] = max(db.signals{s, p}(95:105, b));
                            [y_values(4), x_values(4)] = min(db.signals{s, p}(150:181, b));   
                        end
    
                        x_values(2) = x_values(2) + 74;
                        x_values(3) = x_values(3) + 94;
                        x_values(4) = x_values(4) + 149;
    
                        % Norm data before compute distance
                        x_max = max( [x_values; R_pos]);
                        y_max = max( [y_values; R_value]);
                        x_min = min( [x_values; R_pos]);
                        y_min = min( [y_values; R_value]);
    
                        R_pos = (R_pos - x_min) / (x_max - x_min);
                        R_value = (R_value - y_min) / (y_max - y_min);
    
                        for n = 1:4
                            x_values(n) = (x_values(n) - x_min) / (x_max - x_min);
                            y_values(n) = (y_values(n) - y_min) / (y_max - y_min);
                            x_diff = (R_pos - x_values(n)) ;
                            y_diff = R_value - y_values(n);
                            features(n) = norm([x_diff, y_diff]);
    
                            if isnan(features(n))
                               features(n) = 0.0;
                            end
                        end
    
                        % Add a feature that represent the difference between
                        % the next R-peak?
                        f_wave = [f_wave; features];
                    end
                end
    
                features_wave{p}(1:length(f_wave(:)), b) = f_wave(:);
            end
    
    
            if isempty(db_features)
                db.features_wave{p} = features_wave{p};
            else
                db.features_wave{p} = [db_features{p}; features_wave{p}];
            end
        end 
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
end

% Amplitude
if(sum(morphology_type == 5))
    db_features = db.features_wave;
    features_wave = [];
    % center the signal with the window of size = lag to compute each statistics
    for(p = 1:size(db.signals, 2))
        for(b =1:size(db.signals{1, p}, 2))
            f_wave = [];
            
            for s=1:2
                if signals_used(s)
                    if b > 1
                        pre = db.signals{s, p}(91, b-1);
                    else
                        pre = db.signals{s, p}(91, b);
                    end
                    
                    if b+1 < size(db.signals{1, p}, 2)
                        post = db.signals{s, p}(91, b+1);
                    else
                        post = db.signals{s, p}(91, b);
                    end
                    
                    features = [pre, db.signals{s, p}(91, b), post];
                    f_wave = [f_wave; features];
                end
            end
            
            features_wave{p}(1:length(f_wave(:)), b) = f_wave(:);
        end
        
        if isempty(db_features)
            db.features_wave{p} = features_wave{p};
        else
            db.features_wave{p} = [db_features{p}; features_wave{p}];
        end
    end
end



%% DTW
%if(sum(morphology_type == 6))

%end


% if(sum(morphology_type == 4))
%     % Descriptor binario
%
%     % Considerando ventanas equidistantes al centro se compara desde el
%     % primero de la izquierda con el ultimo de la derecha hasta llegar al
%     % centro... Si es mayor el punto de la izquierda se asigna un 1 sino un
%     % 0. Si trabajmos con 90 y 90 puntos (a cada extremo) esto nos dejaria
%     % con 90 valores binarios que podemos agrupar de 8 en 8 bits
%     % 90/8 ~=11-12 valores enteros. Asi resumimos 180 valores en unos 12
%     % valores enteros...
%
%     bit_sets = 4;
%
%     descriptor_size = ceil(window_l / bit_sets); % 8 (bits) we could use also * 4 bytes (int32) = 16 * 4 = 64
%     descriptor_bit = [];
%     descriptor_dec = [];
%     features_wave{size(db.signals, 2)} = [];
%     beat_size = size(db.signals{1}, 1);
%     for(s = 1:size(db.signals, 2))
%         for(b =1:size(db.signals{s}, 2))
%             for(w =1:window_l)
%                 descriptor_bit(w) = db.signals{s}(w,b) >  db.signals{s}(beat_size-w+1,b);
%             end
%             % Agrup to byte each 8 values
%             for(d =1:descriptor_size)
%                 descriptor_dec(d) = bi2de(descriptor_bit( (((d-1)*bit_sets) + 1 ): (min( (d*bit_sets), size(descriptor_bit, 2))) ));
%             end
%             features_wave{s}(1:length(descriptor_dec), b) = descriptor_dec;
%         end
%     end
%
% end
%
% %% Shape Matching
% if(sum(morphology_type == 5))
%     %Compute mean from interval = 10
%     interval_size = 10;
%
%     features_wave{size(db.signals, 2)} = [];
%
%     for(s = 1:size(db.signals, 2))
%         for(b =1:size(db.signals{s}, 2))
%             index = 0;
%             for(w=1:interval_size:size(db.signals{s}, 1))
%                 num_elems = min(w+interval_size, size(db.signals{s}, 1)) - w;
%                 if(num_elems > 0)
%                     index = index +1;
%                     mean_values(index) = sum( db.signals{s}(w:min(w+interval_size, size(db.signals{s}, 1)), b))/ num_elems;
%                 end
%             end
%             features_wave{s}(1:index, b) = mean_values;
%             mean_values = [];
%         end
%     end
% end
%
% %% Hjorth
% if(sum(morphology_type == 6))
%     features_wave{size(db.signals, 2)} = [];
%
%     for(s = 1:size(db.signals, 2))
%         for(b =1:size(db.signals{s}, 2))
%
%             x0(1) = db.signals{s}(1, b);
%             x0(2) = db.signals{s}(2, b);
%             x0(3) = db.signals{s}(3, b);
%
%             x1(1) = x0(2) - x0(1);
%             x1(2) = x0(3) - x0(2);
%
%             x2(1) = x1(2) - x1(1);
%
%             for(w=4:size(db.signals{s}, 1))
%                 x0(w) = db.signals{s}(w, b);
%                 x1(w-1) = db.signals{s}(w, b) - db.signals{s}(w-1, b);
%                 x2(w-2) = x1(w-1) - x1(w-2);
%             end
%
%             % std(x, w): w = 1 -> then 1/N if not -> 1/(N-1)
%             features_wave{s}(1:3, b) = [std(x0, 1), std(x0, 1), std(x0, 1)];
%         end
%     end
% end

num_classes = 4;

if class_division == 1 %zhang_class
    subclasses{1} = {'N', 'L', 'R'};
    subclasses{2} = {'A', 'a', 'J', 'S', 'e', 'j'};
    subclasses{3} = {'V', 'E'};
    subclasses{4} = {'F'};
    %subclasses{5} = {'P', '/', 'f', 'u'};
    
elseif class_division == 2 %binary
    num_classes = 2;
    subclasses{1} = {'N', 'L', 'R'};
    subclasses{2} = {'A', 'a', 'J', 'S', 'V', 'E', 'F',  'e', 'j'};
    
elseif class_division == 3 %considering all subclasses
    
    num_classes = 12;
    
    subclasses{1} = {'N'};
    subclasses{2} = {'L'};
    subclasses{3} = {'R'};
    subclasses{4} = {'A'};
    subclasses{5} = {'a'};
    subclasses{6} = {'J'};
    subclasses{7} = {'S'};
    subclasses{8} = {'e'};
    subclasses{9} = {'j'};
    subclasses{10} = {'V'};
    subclasses{11} = {'E'};
    subclasses{12} = {'F'};
    
elseif class_division == 4 % my test maintaining only class that are similars to the super class or that contains enough number at train/test division
    subclasses{1} = {'N'};
    subclasses{2} = {'A', 'a'};
    subclasses{3} = {'V'};
    subclasses{4} = {'F'};
    
else % chazal
    subclasses{1} = {'N', 'L', 'R', 'e', 'j'};
    subclasses{2} = {'A', 'a', 'J', 'S'};
    subclasses{3} = {'V', 'E'};
    subclasses{4} = {'F'};
    %subclasses{5} = {'P', '/', 'f', 'u'};
    
end

index_per_record{size(patient_list,2), num_classes} = [];

data = [];
label = [];
%test_label_per_record = [];
b_i = 0;
if(norm_RR)
    max_temp_f = 1000;% definimos este valor suponiendo el max de valor temporal
else
    max_temp_f = 1;
end

%tp_i = 0;
%if isempty(db.features_wave{p}) == false

for(p = 1:size(db.signals,2))
    
    %% Compute AVG values
    pre_R_avg = mean(db.temporal_features{p}.pre_R);
    post_R_avg = mean(db.temporal_features{p}.post_R);
    local_R_avg = mean(db.temporal_features{p}.local_R);
    global_R_avg = mean(db.temporal_features{p}.global_R);
    
    features_patient = db.features_wave{p};
    
    if(use_RR)
        features_patient = [features_patient;
            db.temporal_features{p}.pre_R/ max_temp_f;
            db.temporal_features{p}.post_R / max_temp_f;
            db.temporal_features{p}.local_R / max_temp_f;
            db.temporal_features{p}.global_R/ max_temp_f];
        
        % Rhythm Labeling
        % (Amp_pre, Amp_post, Amp, RR_interval, . )
        % seven rhythm labels to discern between four beat rhythm types
        
    end
    
    if(use_normalized_RR)
        features_patient = [features_patient; % And normalized_RR    featureRR/avg(featureRR)   *avg by patient
            db.temporal_features{p}.pre_R / pre_R_avg;
            db.temporal_features{p}.post_R / post_R_avg;
            db.temporal_features{p}.local_R / local_R_avg;
            db.temporal_features{p}.global_R / global_R_avg];
    end
    
    if(use_HB && use_ecgpuwave)
        features_patient = [features_patient; % And normalized_RR    featureRR/avg(featureRR)   *avg by patient
            db.HB_intervals{p}.qrs_durations;
            db.HB_intervals{p}.t_durations;
            db.HB_intervals{p}.t_nums;
            db.HB_intervals{p}.qrs_t_spaces;
            db.HB_intervals{p}.p_flags
            db.HB_intervals{p}.heart_axis];
        
    elseif( use_HB && use_construe)
        features_patient = [features_patient; % And normalized_RR    featureRR/avg(featureRR)   *avg by patient
            db.HB_intervals{p}.p_widths;
            db.HB_intervals{p}.pq_intervals;
            db.HB_intervals{p}.t_widths;
            db.HB_intervals{p}.st_intervals;
            db.HB_intervals{p}.qrs_widths];
    end
    
    data = [data [features_patient]];
    
end
%end

selected_beat = [];

for(p = 1:size(db.signals,2))
    for(b=1:size(db.classes{p},2))
        select = 0;
        
        for(n=1:num_classes) % +1
            if(sum(strcmp(db.classes{p}(b) , subclasses{n})))
                label = [label n];
                index_per_record{p,n} = [index_per_record{p,n}, b];
                % signals_per_class{n} = [signals_per_class{n} db.signals{1,p}(:,b)];
                % label_per_record{tp_i} = [label_per_record{tp_i} n]; % Only for test mitdb!!
                select = 1;
            end
        end
        selected_beat = [selected_beat, select];
    end
end

data = data';
data = data(find(selected_beat == 1),:);

end