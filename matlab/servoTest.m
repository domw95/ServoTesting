%% parameters
clear
close all
sample_rate = 100;


s = serialport("COM6",115200);

%% Get calibration data
calibration1 = [];
l = readline(s);
fprintf("%s\n",l);
if strtrim(l) ~= "calibration1"
    fprintf("Error, expected calibration1\n")
    return
end

while true
    l = readline(s);
    fprintf("%s\n",l)
    if strtrim(l) == "end"
        break
    end
    values = str2num(l); %#ok<ST2NM> 
    
    calibration1(end+1,:) = values;

    
end

calibration2 = [];
l = readline(s);
fprintf("%s\n",l);
if strtrim(l) ~= "calibration2"
    fprintf("Error, expected calibration2\n")
    return
end

while true
    l = readline(s);
    fprintf("%s\n",l)
    if strtrim(l) == "end"
        break
    end
    values = str2num(l); %#ok<ST2NM> 
    
    calibration2(end+1,:) = values;

    
end


%% Get step response data
step_data = [];
l = readline(s);
fprintf("%s\n",l);
if strtrim(l) ~= "step"
    fprintf("Error, expected step\n")
    return
end

while true
    l = readline(s);
    disp(l)
    if numel(l) == 0 || strtrim(l) == "end"
        break
    end
    values = str2num(l); %#ok<ST2NM> 
    
    step_data(end+1,:) = values;

    
end



%% Get frequency data
freq_data = [];
l = readline(s);
fprintf("%s\n",l);
if strtrim(l) ~= "frequency"
    fprintf("Error, expected frequency\n")
    return
end

while true
    l = readline(s);
    disp(l)
    if numel(l) == 0 || strtrim(l) == "end"
        break
    end
    values = str2num(l); %#ok<ST2NM> 
    
    freq_data(end+1,:) = values;

    
end
clear s


%% Data Processing

%% Calibration
% Convert calibration data to angles (degrees)
calibration1(:,3) = calibration1(:,2) * 90/1024;
calibration2(:,3) = calibration2(:,2) * 90/1024;

p1 = polyfit(calibration1(:,1)-3000,calibration1(:,3),1);
p2 = polyfit(calibration2(:,1)-3000,calibration2(:,3),1);

demand_factor = mean([p1(1),p2(1)]);

fprintf("Range of motion is +- %.1f degrees\n",demand_factor*1000)

figure(1);clf;
plot(calibration1(:,1),calibration1(:,3))
hold on
plot(calibration2(:,1),calibration2(:,3))
hold off
grid on

%% Step responses
step_demand = (step_data(:,1) - 2000) * demand_factor;
step_response = 1000*demand_factor + step_data(:,2) * 90/1024;

step_amplitudes = unique(step_demand);

figindex = 1;
for n = 1:numel(step_amplitudes)
    amp = step_amplitudes(n);
    time = (0:numel(step_demand(step_demand==amp))-1)*1/sample_rate;

    % calc velocity
    velocity = gradient(step_response(step_demand==amp),1/sample_rate);

    figindex  = figindex +1;
    figure(figindex);clf;
    yyaxis left
    plot(time, [step_demand(step_demand==amp),step_response(step_demand==amp)])
    ylabel("Position / degrees")
    hold on
    yyaxis right
    plot(time,velocity)
    grid on
    xlabel("Time / s")
    ylabel("Velocity / degrees/s")
    legend(["Step","Response","Velocity"])
    
end

peak_velocity = max(velocity);
speed_rating = 60/peak_velocity;

%% Process frequency data
freq_data(:,5) = (freq_data(:,1))*demand_factor;
freq_data(:,6) = freq_data(:,2)/10;
freq_data(:,7) = (freq_data(:,3)-3000)*demand_factor;
% Scale encoder reading
freq_data(:,8) = freq_data(:,4)*90/1024;

testAmplitudes = unique(freq_data(:,5));
testFreqs = unique(freq_data(:,6));

% phase and amplitude of demand and response
phid = zeros([numel(testAmplitudes),numel(testFreqs)]);
phir = zeros(size(phid));


ampd = zeros(size(phid));
ampr = zeros(size(phid));

for i = 1:numel(testAmplitudes)
    for j = 1:numel(testFreqs)
        amp = testAmplitudes(i);
        freq = testFreqs(j);

        % get the test data for this amp and freq
        ind = freq_data(:,5)==amp & freq_data(:,6)==freq;
        demand = freq_data(ind,7);
        response = freq_data(ind,8);

        

        n = numel(demand);
        time = (1/sample_rate)*(0:n-1)';

%         figindex = figindex + 1;
%         figure(figindex)

%         plot(time,[demand,response])

        [p,a] = dft(demand, time, freq*2*pi);
        phid(i,j) = p;
        ampd(i,j) = a;
        [p,a] = dft(response, time, freq*2*pi);
        phir(i,j) = p;
        ampr(i,j) = a;
    end
end
amp = ampr./ampd;
phi = phir - phid;

response = amp.*exp(1j*phi);
figindex = figindex + 1;
figure(figindex);clf;hold on

options = bodeoptions;
options.FreqUnits = 'Hz'; % or 'rad/second', 'rpm', etc.

for i = 1:numel(testAmplitudes)
    sys = idfrd(response(i,:),testFreqs*2*pi,0);
    bode(sys,options)
end
grid on
legend(num2str(testAmplitudes,'%.1f'),'Location','SouthWest')





