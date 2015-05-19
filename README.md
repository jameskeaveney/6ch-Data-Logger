# 6 channel data logger using Arduino DUE and python
A six-channel data logger running on the Arduino DUE, that sends data via SerialUSB to a PC. Python is used to grab this data, process it and save it.

Original purpose is as a slow multi-channel data logger for use with a DFB laser, where 
a frequency sweep is controlled via temperature tuning. A digital pin is used, with an external
potentiometer, to set the magnitude of the temperature sweep (the onboard DAC could have been used,
but the adjustment range is only 1/6 to 5/6 of the full 0 - 3V3 range). The temperature is swept
by switching the state of the digital pin, which in turn changes the setpoint on the external
temperature controller (Thorlabs TED200C). Since the temperature scan is slow (~90 seconds), and 
we need lots of channels, a normal oscilloscope isn't the best for this job.

Photodiode signals from the main experiment vapour cell, two reference cells (one at zero field,
one at 0.6 Tesla), Fabry-Perot etalon, a laser power monitor photodiode, and the temperature
monitor (actual temperature, rather than setpoint) are required.
