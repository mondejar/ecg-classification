function [data] = extract_features( input, N, family, remove_noise, verbose )

data = [];
for f = 1:length(input)
    if(verbose)
        clf; % Clean plot
    end
    % Signal
    file = dir([input(f).name, '/*.txt']);
    machine_capture = file.name(length(file.name)-5:length(file.name)-4);
    
    if(strcmp(machine_capture, 'PV'))
        original_freq = 200;
    else
        original_freq = 200;
    end
    
    signal = load([input(f).name, '/', file.name]);
    
    % .PV  200HZ
    % .PB .PL .PM 300HZ   
    
    % Metadata
    file = dir([input(f).name, '/*.json']);
    fileID = fopen([input(f).name, '/', file.name]); 
    metadata = fgets(fileID);
    
    % Find "tipo":"II"  begin
    l = strfind(metadata, '"inicioVentana":');
    begin_type_II = str2num(metadata(l(2)+16:l(3)-16));
    begin_type_III = str2num(metadata(l(3)+16:l(4)-17));
    
    %% TODO: depurar el inicio/fin de derivacion II
    signal_II = signal(begin_type_II:begin_type_II + original_freq * 5); %begin_type_II:begin_type_III); 

    %% Norm values in 0-1 
    % Importante realizar este paso!!
    max_Amp = 255;
    min_Amp = 0;
    signal_II = (signal_II - min_Amp) / (max_Amp - min_Amp); % normalize between 0,1
    
    %% Convert the signal from (200-300hz) to 360hz
    % 200/300
    fs = 360;
    [P,Q] = rat(fs/original_freq);
    
    signal_II = resample(signal_II,P,Q);
    if(original_freq == 200)
        signal_II = signal_II(1:length(signal_II)-2);% mete algo de ruido al ahcer el resample completo
    else
        signal_II = signal_II(1:length(signal_II)-3);
    end        
    num_features = size(signal_II, 1);
        
    if(verbose)
        subplot(4,1,1);
        plot(signal_II);
    end

    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
    if(remove_noise)
        %% Remove baseline 
        %%%%%%%% Multilevel decomposition
        % Perform a multilevel wavelet decomposition of a signal.
        [C, L] = wavedec(signal_II, N, family);
        %Aproximation, Detailed

        %To extract the level N approximation coefficients from C, type
        cA_N = appcoef(C, L, family, N);

        % Reconstruct the Level N approximation 
        A_N = wrcoef('a', C, L, family, N);
        corrected_signal = signal_II - A_N;

        if(verbose)
            subplot(4, 1,2);
            plot(corrected_signal);
        end

        %% Remove noise high frecuency
        % Frequency-selective signal filtering was implemented using a set of adaptive bandstop and lowpass filters 
        fc = fs/30; % Cut off frequency %sometimes fs = 200, fs = 300

        % Butterworth filter of order 6
        % Low pass filter: remove high frequencies
        [b,a] = butter(6, fc/(fs/4)); 
        corrected_signal_filt = filter(b, a, corrected_signal); % Will be the filtered signal

        if(verbose)
            subplot(3,1,3);
            plot(corrected_signal_filt);
        end

        data = [data corrected_signal_filt];
    else
        
        %% TODO aqui falla cuando agrupamos señales de diferentes frecuencias... tiene sentido
        % hay que controlarlo antes y fijar la ventana segun el tipo de
        % frecuencia ese + 1100 que sea dependiente de la frecuencia...
        
        data = [data signal_II];        
    end
end