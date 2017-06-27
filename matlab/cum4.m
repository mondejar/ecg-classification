function cum=cum4(signal,maxlag)

% CUM=CUM4(SIGNAL,MAXLAG)
%
% Computes 4th order cumulant biased estimate from signal matrix and maximum lag
% value inputs.
%
% Input signal matrix contains samples in rows and records in columns or is a row
% vector.
%
% maxlag < size(signal,1) for matrix signal or maxlag < length(signal) for vector
% signal.  If unspecified, maxlag = 0.

% Implemented using MATLAB 5.2.1
% Calls MATLAB Signal Processing Toolbox 4.1 function xcorr.m
% Copyright (c) 1988-98 by The MathWorks, Inc.
%
% Implementation:
%
% cum(k,l,m)=sum_{n=0}^{N-1} (x(n)*conj(x(n+k))*conj(x(n+l))*conj(x(n+m)))/N
%
% - [(sum_{n=0}^{N-1} (x(n)*conj(x(n+k)))
%    *sum_{n=0}^{N-1} (conj(x(n+l))*conj(x(n+m))))
%
% + (sum_{n=0}^{N-1} (x(n)*conj(x(n+l)))
%   *sum_{n=0}^{N-1} (conj(x(n+k))*conj(x(n+m))))
%
% + (sum_{n=0}^{N-1} (x(n)*conj(x(n+m)))
%   *sum_{n=0}^{N-1} (conj(x(n+k))*conj(x(n+l))))]/N^2
%
% Examples:
%
% >> x=[1-i -1+i]
%
% x =
%
%   1.0000 - 1.0000i  -1.0000 + 1.0000i
%
% >> y=cum4(x,1)
% record 1: time = 0.05 endseconds
% (3,3,3) cumulant computed in 0.06 seconds
%
% y(:,:,1) =
%
%  -0.0000 + 4.0000i   0.0000 - 4.0000i   0.0000 + 2.0000i
%   0.0000 - 4.0000i  -0.0000 + 4.0000i   0.0000 - 2.0000i
%   0.0000 + 2.0000i        0 - 2.0000i  -0.0000 + 2.0000i
%
% y(:,:,2) =
%
%   0.0000 - 4.0000i  -0.0000 + 4.0000i   0.0000 - 2.0000i
%  -0.0000 + 4.0000i        0 - 8.0000i   0.0000 + 4.0000i
%  -0.0000 - 2.0000i   0.0000 + 4.0000i  -0.0000 - 4.0000i
%
% y(:,:,3) =
%
%   0.0000 + 2.0000i        0 - 2.0000i  -0.0000 + 2.0000i
%  -0.0000 - 2.0000i   0.0000 + 4.0000i  -0.0000 - 4.0000i
%  -0.0000 + 2.0000i  -0.0000 - 4.0000i   0.0000 + 4.0000i
%
% >> x=[1+i 0 0 -1-i]
%
% x =
%
%   1.0000 + 1.0000i        0                  0            -1.0000 - 1.0000i
%
% >> y=cum4(x)
%
% y =
%
%        0 + 1.0000i
%
% >> x=[1+i 0 0 -1-i;0 1-i -1+i 0].'
%
% x =
%
%   1.0000 + 1.0000i        0
%        0             1.0000 - 1.0000i
%        0            -1.0000 + 1.0000i
%  -1.0000 - 1.0000i        0
%
% >> y=cum4(x)
%
% y =
%
%     0
%
% Reference:
%
% C. L. Nikias, A. P. Petropulu, "Higher-Order Spectra Analysis:  A Nonlinear Signal
% Processing Framework", PTR Prentice Hall, Englewood Cliffs, NJ, 1993.
%
%---------------------
% Copyright (c) 1998
% Tom McMurray
% mcmurray@teamcmi.com
%---------------------

[sample,record]=size(signal);
if sample==1
    sample=record;
    record=1;
    signal=signal.';
end
if nargin==1
    maxlag=0;
end
sample1=sample-1;
if maxlag>sample1
    disp(['modifying maximum lag = ' num2str(maxlag)...
        ' to signal sample length - 1 = ' num2str(sample1)])
    maxlag=sample1;
end

%	compute constants

sampls1=sample+1;
sample21=sample*2-1;
maxlag1=maxlag+1;
maxlag12=maxlag1*2;
maxlag2=maxlag*2;
maxlag21=maxlag2+1;
maxlag31=maxlag+maxlag21;
maxlag32=maxlag31+1;
sampmaxl=sample+maxlag;
sammmaxl=sample-maxlag;
sampml21=sample21+maxlag2;
sammml21=sample21-maxlag2;

%	subtract mean from signal

meansig=mean(signal);
signal=signal-meansig(ones(sample,1),:);

%	initialize cumulant array

cum=zeros(maxlag21,maxlag21,maxlag21);

%	for maxlag = 0, compute scalar cumulant and return

if ~maxlag
    for m=1:record
        sig=signal(:,m);
        conjsig=conj(sig);
        cum=cum+(sig.*sig)'*(sig.*conjsig)/sample-sig'*sig*sig'*conjsig*3/sample/sample;
    end
    cum=cum/record;
    return
end

%	signal record loop, maxlag > 0

tic
for m=1:record
    time=cputime;
    sig=signal(:,m);
    conjsig=conj(sig);
    flipudsig=flipud(sig);
    zerosam1=zeros(sample1,1);
    conjsig0=[conjsig;zerosam1];
    
    %	generate 2nd order cumulants and cumulant matrix for subsequent computations
    
    cov1=xcorr(conjsig0);
    cov1=cov1(sammml21:sampml21).'/sample;
    cov2=xcorr([sig;zerosam1],conjsig0);
    cov2=cov2(sammml21:sampml21).'/sample;
    cumat1=zeros(sample21,sample);
    
    %	compute cum(k,l,0)
    
    for k=1:sample
        cumat1(k:sample1+k,k)=flipudsig*conjsig(k)*sig(k);
    end
    cumat1=cumat1(sammmaxl:sampmaxl,:);
    for k=1:maxlag21
        cum(k,:,maxlag1)=cum(k,:,maxlag1)...
            +xcorr(cumat1(maxlag12-k,:),conjsig,maxlag,'biased')...
            -cov1(maxlag21)*cov2(maxlag2+k:-1:k)...
            -cov1(maxlag+k)*cov2(maxlag31:-1:maxlag1)...
            -cov2(maxlag32-k)*cov1(maxlag1:maxlag31);
    end
    
    %	compute cum(k,l,m), -maxlag<m<maxlag, m~=0
    
    for l=1:maxlag
        cumat1=zeros(sample21,sample);
        cumat2=cumat1;
        for k=1:sample-l
            sampls1k=sampls1-k;
            cumat1(k:sample1+k,k)=flipudsig*conjsig(k)*sig(k+l);
            cumat2(sampls1k:sample1+sampls1k,sampls1k)=...
                flipudsig*conjsig(sampls1k)*sig(sampls1k-l);
        end
        cumat1=cumat1(sammmaxl:sampmaxl,:);
        cumat2=cumat2(sammmaxl:sampmaxl,:);
        for k=1:maxlag21
            maxlagk=maxlag+k;
            maxlag12k=maxlag12-k;
            maxlag2k=maxlag2+k;
            maxlag32k=maxlag32-k;
            maxlag1pl=maxlag1+l;
            maxlag1ml=maxlag1-l;
            cum(k,:,maxlag1pl)=cum(k,:,maxlag1pl)...
                +xcorr(cumat1(maxlag12k,:),conjsig,maxlag,'biased')...
                -cov1(maxlag21+l)*cov2(maxlag2k:-1:k)...
                -cov1(maxlagk)*cov2(maxlag31+l:-1:maxlag1+l)...
                -cov2(maxlag32k+l)*cov1(maxlag1:maxlag31);
            cum(k,:,maxlag1ml)=cum(k,:,maxlag1ml)...
                +xcorr(cumat2(maxlag12k,:),conjsig,maxlag,'biased')...
                -cov1(maxlag21-l)*cov2(maxlag2k:-1:k)...
                -cov1(maxlagk)*cov2(maxlag31-l:-1:maxlag1-l)...
                -cov2(maxlag32k-l)*cov1(maxlag1:maxlag31);
        end
    end
    disp(['record ' num2str(m) ': time = ' num2str(cputime-time) ' seconds'])
end
cum=cum/record;
time=num2str(toc);
strmaxlag21=num2str(maxlag21);
disp(['(' strmaxlag21 ',' strmaxlag21 ',' strmaxlag21 ') cumulant computed in '...
    time ' seconds'])

%plot cum(:,:,maxlag1)

lag=-maxlag:maxlag;
if isreal(signal)
    imagesc(lag,lag,cum(:,:,maxlag1))
    title('\fontsize{10} 4^{th} Order Cumulant, \tau_{3} = 0')
else
    imagesc(lag,lag,abs(cum(:,:,maxlag1)))
    title('\fontsize{10} 4^{th} Order Cumulant Magnitude, \tau_{3} = 0')
end
xlabel('\fontsize{10} \tau_{1}')
ylabel('\fontsize{10} \tau_{2}')
axis xy
grid
colormap(gray)
colorbar
end

