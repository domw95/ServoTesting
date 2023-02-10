clear; close all;

load('corona5vtest')

figure(1);clf;
hold on

options = bodeoptions;
options.FreqUnits = 'Hz'; % or 'rad/second', 'rpm', etc.

sys = idfrd(response(1,:),testFreqs*2*pi,0);
bode(sys,options,'b')
sys = idfrd(response(end,:),testFreqs*2*pi,0);
bode(sys,options,'b--')

load('savox7,5vtest.mat')

sys = idfrd(response(1,:),testFreqs*2*pi,0);
bode(sys,options,'r')
sys = idfrd(response(end,:),testFreqs*2*pi,0);
bode(sys,options,'r--')

legend({'Corona5v','Corona5v','Savox7.5v','Savox7.5v'},'Location','Southwest')
grid on