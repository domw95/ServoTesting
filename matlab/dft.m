% Calculates the phase and amplitude of the input signal at a
% particular frequency using discrete fourier series
%% Args
% x = signal vector
% t = time vector. equal spaced vector with same length as x
% w = frequency rad/s
%% Returns
% phi = phase of signal at frequency w rad/s
% amp = amplitude of signal at frequency w
%%
function [phi,amp] = dft(x,t,w)
    a = (2/numel(t))*sum(x.*cos(w*t));
    b = (2/numel(t))*sum(x.*sin(w*t));
    phi = atan2(a,b);
    amp = sqrt(a^2+b^2);
end