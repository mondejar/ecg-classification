%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Given the data as (data, label) and a trained classifier, this function:
% evaluated the classifier over the input data and return de performance
% measures follows the AAMI recomendations.
%
% Author: Mondejar Guerra, Victor M
% VARPA
% University of A Coruña
% 26 June 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [performance_measures, output] = test_SVM(num_classes, standardization, unit_vector, models_SVM, media, st_desviation, data, label, dir_path, C)
counter_class = zeros(size(data, 1), num_classes);
%result = zeros(num_classes, num_classes);
%predicted_labels = zeros(num_classes, num_classes);

if(standardization)   
    for(i=1:size(data, 2))% for each feature 
        data(:, i) = (data(:, i) - media(i)) / st_desviation(i);
    end
end

% Perform the unit vector so the length vector == 1 to all instances
if(unit_vector)   
    for(i=1:size(tdata,1))
        data(i,:) = data(i,:)/ norm(data(i,:));
    end
end

%% One vs One
%% 1 - 2   N vs S
%% 1 - 3   N vs V
%% 1 - 4   N vs F
%% 2 - 3   S vs V
%% 2 - 4   S vs F
%% 3 - 4   V vs F
counter_class = zeros(size(data, 1), num_classes);
predicted_labels = [];
probs = [];

index = 0;
for(k=1:num_classes-1)
    for(kk= k+1:num_classes )
        index = index + 1;
        [predicted_label, accuracy, prob] = svmpredict(ones(size(data, 1), 1), data, models_SVM{index}, ' -q'); % -b 1
        
        % Puede que los valores de prob se deban a los weights...
        
        %predicted_labels = [predicted_labels; predicted_label];
        %TODO dejarlo como un bucle que incremente en evz de uno, la prob correspondiente en cada caso
        counter_class(find(predicted_label == 1),k) = counter_class(find(predicted_label == 1),k) +1;
        counter_class(find(predicted_label == -1),kk) = counter_class(find(predicted_label == -1),kk) +1;

%         for i=1:length(predicted_label)
%             if(predicted_label(i) == 1)
%                 counter_class(i, k) = counter_class(i, k) + prob(i, 1);
%             else
%                 if k == 1                    
%                     counter_class(i, kk) = counter_class(i, kk) + prob(i, 2);
%                 else
%                     counter_class(i, kk) = counter_class(i, kk) + prob(i, 2);
%                 end
%             end
%         end
        
        predicted_label_by_class{index} = predicted_label;
        probs{index} = prob;
    end
end

for(i = 1: size(data, 1))
    % Check max class voting
    counter_class_r = counter_class(i,:);
    maximum = max(counter_class_r);
    selected_class = find(counter_class_r == maximum);
    if( size(selected_class, 2) > 1)
        
        
        %% TODO: think this step again!
        
        % if draw happen (more than one class with maximum votes)
        % check the comparison between both classes
        %if(result(selected_class(1), selected_class(2)) == 1)
        %    selected_class = selected_class(1);
        %else
        %    selected_class = selected_class(2);
        %end
        
        selected_class = 1;

    end
    output(i) = selected_class;
end

%% COMPUTE PERFORMANCE MEASURES EVALUATION FROM AAMI RECOMENDATIONS
%performance_measures_per_record = evaluation_AAMI_per_record(output, label_per_record);
performance_measures = evaluation_AAMI(output, label);

% Write score
% Also: Export predictions, probs, and counter_class to do late-fusion
[fleiss_kappa_DS2, kappa_DS2, prob_expected_DS2, prob_observed_DS2] = write_to_file(performance_measures, dir_path, C, counter_class, probs, label);   
% Fleiss Kappa as measure for best configuration

end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% name: evaluation_AAMI
% inputs: output, ground_truth
%
% Validate the learning algorithm following the AAMI recommendations for
% gross data. Using sensivity (recall), specificity (precision) and
% accuracy for each class: N, SVEB, VEB, F.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [performance_measures, output] = evaluation_AAMI(output, ground_truth)

num_classes = 4; % 5

%% Compute only one confussion matrix for gross
% Compute confussion matrix
confussion_matrix = zeros(4,4);
for(i = 1: size(output,2))
    if(ground_truth(i) <= num_classes) % NOTE: for not consider Q class
        confussion_matrix(ground_truth(i), output(i)) = confussion_matrix(ground_truth(i), output(i))+1;
    end
end

sum_all = sum(sum(confussion_matrix));

confussion_matrix;

Nn = confussion_matrix(1, 1);
Ns = confussion_matrix(1, 2);
Nv = confussion_matrix(1, 3);
Nf = confussion_matrix(1, 4);

Sn = confussion_matrix(2, 1);
Ss = confussion_matrix(2, 2);
Sv = confussion_matrix(2, 3);
Sf = confussion_matrix(2, 4);

Vn = confussion_matrix(3, 1);
Vs = confussion_matrix(3, 2);
Vv = confussion_matrix(3, 3);
Vf = confussion_matrix(3, 4);

Fn = confussion_matrix(4, 1);
Fs = confussion_matrix(4, 2);
Fv = confussion_matrix(4, 3);
Ff = confussion_matrix(4, 4);

%% N
TNn = Ss + Sv + Sf  + Vs + Vv + Vf + Fs + Fv + Ff;
FNn = Ns + Nv + Nf;
TPn = Nn;
FPn = Sn + Vn + Fn;

Recall_N = TPn / (TPn + FNn);
Precision_N = TPn / (TPn + FPn);

Specificity_N = TNn / (TNn + FPn); % 1-FPR
FPR_N = FPn / (TNn + FPn);

Acc_N = (TPn + TNn) / (TPn + TNn + FPn + FNn);

%% SVEB
TNs = Nn + Nv + Nf + Vn + Vv + Vf + Fn + Fv + Ff;
FNs = Sn + Sv + Sf;
TPs = Ss;
FPs = Ns + Vs + Fs;

Recall_SVEB = TPs / (TPs + FNs);
Precision_SVEB = TPs / (TPs + FPs);
FPR_SVEB = FPs / (TNs + FPs);
Acc_SVEB = (TPs + TNs) / (TPs + TNs + FPs + FNs);

%% VEB
TNv = Nn + Ns + Nf + Sn + Ss + Sf + Fn + Fs + Ff;
FNv = Vn + Vs + Vf;
TPv = Vv;
FPv = Nv + Sv; %Fv

Recall_VEB = TPv / (TPv + FNv);
Precision_VEB = TPv / (TPv + FPv);
FPR_VEB = FPv / (TNv + FPv);
Acc_VEB = (TPv + TNv) / (TPv + TNv + FPv + FNv);

%% F
TNf = Nn + Ns + Nv + Sn + Ss + Sv  + Vn + Vs + Vv;
FNf = Fn + Fs + Fv;
TPf = Ff;
FPf = Nf + Sf + Vf;

Recall_F = TPf / (TPf + FNf);
Precision_F = TPf / (TPf + FPf);
FPR_F = FPf / (TNf + FPf);
Acc_F = (TPf + TNf) / (TPf + TNf + FPf + FNf);

%% Overall ACC
Acc = (TPn + TPs + TPv + TPf) / sum_all;

%% Print results

performance_measures.Recall_N = Recall_N;
performance_measures.Recall_SVEB = Recall_SVEB;
performance_measures.Recall_VEB = Recall_VEB;
performance_measures.Recall_F = Recall_F;

performance_measures.Precision_N = Precision_N;
performance_measures.Precision_SVEB = Precision_SVEB;
performance_measures.Precision_VEB = Precision_VEB;
performance_measures.Precision_F = Precision_F;

performance_measures.FPR_N = FPR_N;
performance_measures.FPR_SVEB = FPR_SVEB;
performance_measures.FPR_VEB = FPR_VEB;
performance_measures.FPR_F = FPR_F;

performance_measures.Acc_N = Acc_N;
performance_measures.Acc_SVEB = Acc_SVEB;
performance_measures.Acc_VEB = Acc_VEB;
performance_measures.Acc_F = Acc_F;

performance_measures.Acc = Acc;
performance_measures.confussion_matrix = confussion_matrix;

end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% name: evaluation_AAMI_per_record
% inputs: output, gt_per_record
%
% Validate the learning algorithm following the AAMI recommendations for
% each record. Using sensivity (recall), specificity (precision) and
% accuracy for each class: N, SVEB, VEB, F.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function performance_measures = evaluation_AAMI_per_record(output, gt_per_record)

num_classes = 4; % 5
j = 0;

for t=1:size(gt_per_record,2)
    % Compute confussion matrix
    confussion_matrix = zeros(4,4);
    
    for(i = 1: size(gt_per_record{t}, 2))
        j = j+1;
        if(gt_per_record{t}(i) <= num_classes) % NOTE: for not consider Q class
            confussion_matrix(gt_per_record{t}(i), output(j)) = confussion_matrix(gt_per_record{t}(i), output(j))+1;
        end
    end
    
    sum_all = sum(sum(confussion_matrix));
    
    Nn = confussion_matrix(1, 1);
    Ns = confussion_matrix(1, 2);
    Nv = confussion_matrix(1, 3);
    Nf = confussion_matrix(1, 4);
    
    Sn = confussion_matrix(2, 1);
    Ss = confussion_matrix(2, 2);
    Sv = confussion_matrix(2, 3);
    Sf = confussion_matrix(2, 4);
    
    Vn = confussion_matrix(3, 1);
    Vs = confussion_matrix(3, 2);
    Vv = confussion_matrix(3, 3);
    Vf = confussion_matrix(3, 4);
    
    Fn = confussion_matrix(4, 1);
    Fs = confussion_matrix(4, 2);
    Fv = confussion_matrix(4, 3);
    Ff = confussion_matrix(4, 4);
    
    %% N
    TNn = Ss + Sv + Sf  + Vs + Vv + Vf + Fs + Fv + Ff;
    FNn = Ns + Nv + Nf;
    TPn = Nn;
    FPn = Sn + Vn + Fn;
    
    Recall_N{t} = TPn / (TPn + FNn);
    Precision_N{t} = TPn / (TPn + FPn);
    FPR_N{t} = FPn / (TNn + FPn);
    Acc_N{t} = (TPn + TNn) / (TPn + TNn + FPn + FNn);
    
    %% SVEB
    TNs = Nn + Nv + Nf + Vn + Vv + Vf + Fn + Fv + Ff;
    FNs = Sn + Sv + Sf;
    TPs = Ss;
    FPs = Ns + Vs + Fs;
    
    Recall_SVEB{t} = TPs / (TPs + FNs);
    Precision_SVEB{t} = TPs / (TPs + FPs);
    FPR_SVEB{t} = FPs / (TNs + FPs);
    Acc_SVEB{t} = (TPs + TNs) / (TPs + TNs + FPs + FNs);
    
    %% VEB
    TNv = Nn + Ns + Nf + Sn + Ss + Sf + Fn + Fs + Ff;
    FNv = Vn + Vs + Vf;
    TPv = Vv;
    FPv = Nv + Sv; %Fv;
    
    Recall_VEB{t} = TPv / (TPv + FNv);
    Precision_VEB{t} = TPv / (TPv + FPv);
    FPR_VEB{t} = FPv / (TNv + FPv);
    Acc_VEB{t} = (TPv + TNv) / (TPv + TNv + FPv + FNv);
    
    %% F
    TNf = Nn + Ns + Nv + Sn + Ss + Sv  + Vn + Vs + Vv;
    FNf = Fn + Fs + Fv;
    TPf = Ff;
    FPf = Nf + Sf + Vf;
    
    Recall_F{t} = TPf / (TPf + FNf);
    Precision_F{t} = TPf / (TPf + FPf);
    FPR_F{t} = FPf / (TNf + FPf);
    Acc_F{t} = (TPf + TNf) / (TPf + TNf + FPf + FNf);
    
    %% Overal ACC
    Acc{t} = (TPn + TPs + TPv + TPf) / sum_all;
end

performance_measures.Recall_N = Recall_N;
performance_measures.Recall_SVEB = Recall_SVEB;
performance_measures.Recall_VEB = Recall_VEB;
performance_measures.Recall_F = Recall_F;

performance_measures.Precision_N = Precision_N;
performance_measures.Precision_SVEB = Precision_SVEB;
performance_measures.Precision_VEB = Precision_VEB;
performance_measures.Precision_F = Precision_F;

performance_measures.FPR_N = FPR_N;
performance_measures.FPR_SVEB = FPR_SVEB;
performance_measures.FPR_VEB = FPR_VEB;
performance_measures.FPR_F = FPR_F;

performance_measures.Acc_N = Acc_N;
performance_measures.Acc_SVEB = Acc_SVEB;
performance_measures.Acc_VEB = Acc_VEB;
performance_measures.Acc_F = Acc_F;

performance_measures.Acc = Acc;

end


% Compute and save performance measures into a file
function [kappaF, kappa, prob_expected, prob_observed] = write_to_file(performance_measures, dir_path, C, counter_class, probs, label_gt)

cm = performance_measures.confussion_matrix;

[kappa, prob_expected, prob_observed] = compute_cohen_kappa(cm);
[kappaF, prob_expectedF, prob_observedF] = compute_fleiss_kappa(cm);

%% Optimization of ECG Classification by Means of Feature Selection (Mar)
%J index
Ij = performance_measures.Recall_SVEB + performance_measures.Recall_VEB + performance_measures.Precision_SVEB + performance_measures.Precision_VEB;

w1 = 0.5;
w2 = 0.125;
Ijk = w1 * kappa + w2 * Ij;

output_name = [dir_path, 'score_', num2str(Ijk, '% 1.3f'), '_C_', num2str(C), '.txt'];

fid = fopen(output_name,'w');
fprintf(fid, 'Confussion Matrix \n');

for( i = 1:size(cm,1))
    fprintf(fid, '%d %d %d %d\n', cm(i,:));
end

fprintf(fid, '\nMar scores Ijk: %f\n', Ijk);
fprintf(fid, 'Ij: %f (max 4)\n', Ij);

fprintf(fid, '\nCohen Kappa Values: %f\n', kappa);
fprintf(fid, 'Prob expected: %f\n', prob_expected);
fprintf(fid, 'Prob Observed (Overal Acc): %f\n', prob_observed);

fprintf(fid, '\nFleiss Kappa Values: %f\n', kappaF);
fprintf(fid, 'Prob expected: %f\n', prob_expectedF);
fprintf(fid, 'Prob Observed: %f\n', prob_observedF);

fprintf(fid, '\nG-mean (Geometric mean)\n'); 
%% TODO for each binary classifier, then average g-means 

fprintf(fid, '\nRecall\n');
fprintf(fid, '%f %f %f %f\n', [performance_measures.Recall_N, performance_measures.Recall_SVEB, performance_measures.Recall_VEB, performance_measures.Recall_F]);

fprintf(fid, '\nPrecision\n');
fprintf(fid, '%f %f %f %f\n', [performance_measures.Precision_N, performance_measures.Precision_SVEB, performance_measures.Precision_VEB, performance_measures.Precision_F]);

fprintf(fid, '\nFPR\n');
fprintf(fid, '%f %f %f %f\n', [performance_measures.FPR_N, performance_measures.FPR_SVEB, performance_measures.FPR_VEB, performance_measures.FPR_F]);

% NOTE: El acc por clase es mala medida. Al existir tanto desbalanceo si se
% deja una clase a 0 se obtiene un buen Acc debido al bajo numero de FN
% respecto TN
% fprintf(fid, '\nAcc\n');
% fprintf(fid, '%f %f %f %f\n', [performance_measures.Acc_N, performance_measures.Acc_SVEB, performance_measures.Acc_VEB, performance_measures.Acc_F]);

% AVG sensivity(recall), specificity, accuracy, +P
avg_recall = sum([performance_measures.Recall_N, performance_measures.Recall_SVEB, performance_measures.Recall_VEB, performance_measures.Recall_F]) / 4;
avg_precision = sum([performance_measures.Precision_N, performance_measures.Precision_SVEB, performance_measures.Precision_VEB, performance_measures.Precision_F]) / 4;
avg_specificity = sum([ 1 - performance_measures.FPR_N, 1 - performance_measures.FPR_SVEB, 1 - performance_measures.FPR_VEB, 1 - performance_measures.FPR_F])/ 4;

% avg_acc = sum([performance_measures.Acc_N, performance_measures.Acc_SVEB, performance_measures.Acc_VEB, performance_measures.Acc_F]) / 4;

fprintf(fid, '\nAVG values sum(classes) / num_classes\nRecall Precision Specificity\n');
fprintf(fid, '%f %f %f %f\n', [avg_recall, avg_precision, avg_specificity]);

fclose(fid);

save([dir_path, 'predictions_C_', num2str(C), '.mat'], 'counter_class', 'probs', 'label_gt');

end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Compute Cohen' kappa from a confussion matrix
% Kappa value:
%    < 0.20  Poor
% 0.21-0.40  Fair
% 0.41-0.60  Moderate
% 0.61-0.80  Good
% 0.81-1.00  Very good
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [kappa, prob_expected, prob_observed] = compute_cohen_kappa(cm)
prob_expectedA = [];
prob_expectedB = [];
prob_observed = 0;
for(n = 1:size(cm,1))
    prob_expectedA(n) = sum(cm(n,:))/sum(sum(cm));
    prob_expectedB(n) = sum(cm(:,n))/sum(sum(cm));
    
    prob_observed = prob_observed + cm(n,n);
end

prob_expected = sum(prob_expectedA .* prob_expectedB);
prob_observed = prob_observed / sum(sum(cm));

kappa = (prob_observed - prob_expected) / (1 - prob_expected);
end

% Fleiss Kappa
function [kappa, prob_expected, prob_observed] = compute_fleiss_kappa(cm)
prob_i = [];
prob_i = [];
prob_observed = 0;
for(n = 1:size(cm,2))
    prob_j(n) =  sum(cm(:,n))/sum(sum(cm));  
end

for(m = 1:size(cm,1))
    prob_i(m) = 0;
    if(m > 1)
        for(n = 1:size(cm,2))
            prob_i(m) = prob_i(m) + cm(m,n) * cm(m,n);
        end
    prob_i(m) = (prob_i(m) - sum(cm(m-1,:))) / ( sum(cm(m-1,:)) * (sum(cm(m-1,:))-1));
    
    else
        prob_i(m) = 1.0;
    end
end

prob_expected = sum(prob_j .* prob_j);
prob_observed = sum(prob_i) / size(cm,1);

kappa = (prob_observed - prob_expected) / (1 - prob_expected);
end




function [k, sum_diag, PD, XTot] = compute_kappa_mar(cm)
sum_diag = 0;
PD = 0;

XTot = sum(sum(cm));

for(n = 1:size(cm,1))
    sum_diag = sum_diag + cm(n,n);
    
    OI = sum(cm(n, :));
    AI = sum(cm(:, n));
    PD = PD +  (OI * AI)/ XTot;
end

k = (sum_diag - PD ) / (XTot - PD);

end

