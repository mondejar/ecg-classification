function [data, label, label_per_record] = load_dataset(dataset_name, patient_list, window_l, window_t, morphology_type, use_RR, use_normalized_RR, norm_RR, binary_problem)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Load dataset and set as matrix with data and label vector:
% features selection
% select patients (optionally)
% 
% Mondejar Guerra, Victor M
% 26 Jun 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

path_dataset = '/home/mondejar/dataset/ECG/';
full_path = [path_dataset, dataset_name, '/m_learning/'];
load([full_path, 'data_w_', num2str(window_l), '_', num2str(window_t)]);

if(isempty(patient_list) == false)
    datasetAux.patients = [];
    for p=1:size(patient_list,2)
        pos = find(db.patients == patient_list(p));
        
        datasetAux.patients = [datasetAux.patients patient_list(p)];
        datasetAux.signals{p} = db.signals{pos};
        datasetAux.classes{p} = db.classes{pos};
        datasetAux.selected_R{p} = db.selected_R{pos};
        datasetAux.temporal_features{p} = db.temporal_features{pos};       
    end
    
    datasetAux.window_l = db.window_l;
    datasetAux.window_t = db.window_t;
    
    db = datasetAux;
end

if( strcmp(dataset_name, 'mitdb'))
    %% Norm values in 0-1
    max_Amp = 2048;
    min_Amp = 0;

    for(p = 1:size(db.signals, 2))
        db.signals{p}  = ([db.signals{p}]  - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
    end
    
elseif( strcmp(dataset_name, 'incartdb'))
    %% Norm values in 0-1
    max_Amp = 4000;
    min_Amp = -4000;

    for(p = 1:size(db.signals, 2))
        db.signals{p}  = ([db.signals{p}]  - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
    end    
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% 1 Feature extraction (Create descriptor)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% no morphology
if (morphology_type == -1 )
    features_wave{size(db.signals, 2)} = [];
    db.features_wave = features_wave;
end

%% RAW Signal
if(morphology_type == 0 )
    features_wave = db.signals;
end

%% Wave Signal
if(morphology_type == 1 )
    % Wavelets
    features_wave{size(db.signals, 2)} = [];
    
    for(s = 1:size(db.signals, 2))
        for(b =1:size(db.signals{s}, 2))
            [C, L] = wavedec(db.signals{s}(:, b), 2, 'db1');
            app_w = appcoef(C, L, 'db1', 2);
            features_wave{s}(1:length(app_w), b) = app_w;
        end
    end     
end

%% Chazal Morph
if(morphology_type == 2)
    % chazal morph
    features_wave{size(db.signals, 2)} = [];
    for(s = 1:size(db.signals, 2))
        for(b =1:size(db.signals{s}, 2))
            % (-50 - 100)ms         (150 - 500)ms
            % 1:54 samples          72:198 samples
            % 10 features at 60hz   8 features at 20hz
            i1 = round(1:5.4:54);
            f1 = db.signals{s}(1:54, b);
                
            i2 = round(1:15.75:126);
            f2 = db.signals{s}(72:198, b);
                
            morph = [f1(i1); f2(i2)];
            % morph = ...([db.signals{s}(:, b))
            features_wave{s}(1:length(morph), b) = morph;
        end
    end        
end

%% HOS
if(morphology_type == 3)
    % TODO transform signal to have zero-mean
    
end

%% my morphology
if(morphology_type == 4)
    % para cada punto comprueba su decrece o sube y suma la distancia
    % euclidea
    
    % simplemente habra dos features, 1: subida
    %                                 2: bajada
    features_wave{size(db.signals, 2)} = [];
    
    for(s = 1:size(db.signals, 2))
        for(b =1:size(db.signals{s}, 2))
            up = 0;
            down = 0;
            for(w=1:size(db.signals{s}, 1)-1)
                if(db.signals{s}(w, b) > db.signals{s}(w+1, b))
                    down = down + (db.signals{s}(w, b) - db.signals{s}(w+1, b));
                else
                    up = up + (db.signals{s}(w+1, b) - db.signals{s}(w, b));
                end
            end
            features_wave{s}(1:2, b) = [up, down];
        end
    end
end

%% Shape Matching
if(morphology_type == 5)
    %Compute mean from interval = 10
    interval_size = 10;
    
    features_wave{size(db.signals, 2)} = [];
    
    for(s = 1:size(db.signals, 2))
        for(b =1:size(db.signals{s}, 2))
            index = 0;
            for(w=1:interval_size:size(db.signals{s}, 1))
                num_elems = min(w+interval_size, size(db.signals{s}, 1)) - w;
                if(num_elems > 0)
                    index = index +1;
                    mean_values(index) = sum( db.signals{s}(w:min(w+interval_size, size(db.signals{s}, 1)), b))/ num_elems;
                end
            end
            features_wave{s}(1:index, b) = mean_values;
            mean_values = [];
        end
    end
end

db.features_wave = features_wave;

if binary_problem == false
    num_classes = 4;
    subclasses{1} = {'N', 'L', 'R', 'e', 'j'};
    subclasses{2} = {'A', 'a', 'J', 'S'};
    subclasses{3} = {'V', 'E'};
    subclasses{4} = {'F'};
    %subclasses{5} = {'P', '/', 'f', 'u'};
else
    num_classes = 2;
    subclasses{1} = {'N', 'L', 'R', 'e', 'j'};
    subclasses{2} = {'A', 'a', 'J', 'S', 'V', 'E', 'F'};
end
   
data = [];
label = [];
%test_label_per_record = [];

b_i = 0;
if(norm_RR)
    max_temp_f = 1000;% definimos este valor suponiendo el max de valor temporal
else
    max_temp_f = 1;
end

tp_i = 0;

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
    end
        
    if(use_normalized_RR)
        features_patient = [features_patient; % And normalized_RR    featureRR/avg(featureRR)   *avg by patient
        db.temporal_features{p}.pre_R / pre_R_avg;
        db.temporal_features{p}.post_R / post_R_avg;
        db.temporal_features{p}.local_R / local_R_avg;
        db.temporal_features{p}.global_R / global_R_avg];
    end
        
    data = [data [features_patient]];
        
    for(b=1:size(db.classes{p},2))
        for(n=1:num_classes) % +1
            if(sum(strcmp(db.classes{p}(b) , subclasses{n})))
                label = [label n];
                label_per_record{tp_i} = [label_per_record{tp_i} n]; % Only for test mitdb!!
            end
        end 
    end
end

end