%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Validate the learning algorithm following the AAMI recommendations for
% each record. Using sensivity (recall), specificity (precision) and 
% accuracy for each class: N, SVEB, VEB, F.
%
%
% Author: Mondejar Guerra
% VARPA
% University of A Coruña
% Creation: June 2017
% Last modification: June 2017
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

