clear
close all
s = serialport("COM6",115200);
data = [];

while true
    l = readline(s);
    disp(l)
    if numel(l) == 0 || strtrim(l) == "end"
        break
    end
    values = str2num(l); %#ok<ST2NM> 
    
    data(end+1,:) = values;

    
end
clear s

%% Adjust and scale demand signal 
data(:,2) = (data(:,2)-3000)*90/1000;
% Scale encoder reading
data(:,3) = data(:,3)*60/1024;
% Extract test freqs
data(:,1) = data(:,1)/10;
testFreqs = unique(data(:,1));

%% Analyse data
phi = zeros(size(testFreqs));
amp = zeros(size(testFreqs));



for i = 1:numel(testFreqs)
    freq = testFreqs(i);
    % Get all the data at this frequency
    testData = data(data(:,1)==freq,2:3);
    n = numel(testData(:,1));
    time = 0.02*(0:n-1)';

    figure(i)
    plot(time,testData)
    grid on
    % Demand and response freq and phase
    [phid,ampd] = dft(testData(:,1),time,freq*2*pi);
    [phir,ampr] = dft(testData(:,2),time,freq*2*pi);
    phi(i) = phir-phid;
    amp(i) = ampr/ampd;


end

response = amp.*exp(1j*phi);
sys = idfrd(response,testFreqs*2*pi,0);

figure(i+1)
bode(sys)
grid on
