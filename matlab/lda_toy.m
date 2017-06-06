%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Linear Discriminant Analysis (LDA) 
% Toy example
% 2017
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

load fisheriris;

MdlLinear = fitcdiscr(meas,species);

MdlQuadratic = fitcdiscr(meas,species,'DiscrimType','quadratic');
meanclass2 = predict(MdlQuadratic,meanmeas)
