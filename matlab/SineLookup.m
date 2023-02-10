%% Creates a lookup sine lookup table
MAX_VALUE = 100;
nPOINTS = 256;

points = 1:nPOINTS;
rads = linspace(0,pi/2,nPOINTS);

values = MAX_VALUE*sin(rads);

round_values = round(values);

plot(points,[values;round_values])
grid on

% Sample time of sine generation
Fs = 50;

% Possible jumps
jumps = 1:nPOINTS;

% Possible frequencies
Fp = jumps*Fs/(4*nPOINTS);

figure(2)
plot(Fp)