%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Validate the learning algorithm following the AAMI recommendations for
% gross data. Using sensivity (recall), specificity (precision) and 
% accuracy for each class: N, SVEB, VEB, F.
%
% Author: Mondejar Guerra
% VARPA
% University of A Coruña
% Creation: June 2017
% Last modification: June 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function performance_measures = evaluation_AAMI(output, ground_truth)

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

confussion_matrix

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

Specificity = TNn / (TNn + FPn); % 1-FPR
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

% 	fid = fopen(['output/one_vs_one_C-', num2str(C) , '.txt'],'wt');  % Note the 'wt' for writing in text mode
% 	for(i=1:size(confussion_matrix, 1))
% 		for(j=1:size(confussion_matrix, 2))
% 			fprintf(fid,'%d ', confussion_matrix(i,j));
% 		end
% 		fprintf(fid,'\n');
% 	end
% 	fclose(fid);

% max_avg_acc = 0;
% best_configuration = 0;
% % NOTE Compute best configuration as AVG_acc_class * total_acc
% for(i=1:size(all_accuracy,2))
%     accuracy_c = all_accuracy{i};
%     if( (accuracy_c(num_classes+1) * accuracy_c(num_classes+2))> max_avg_acc)
%         max_avg_acc =  (accuracy_c(num_classes+1) * accuracy_c(num_classes+2));
%         best_configuration = i;
%     end
% end
%
% disp(['Best configuration-> C = ', num2str(list_C_values(best_configuration))]);
% all_accuracy{best_configuration}
% max_avg_acc