import SixChannelReader as SCR
import matplotlib.pyplot as plt
import numpy as np

def main():
	""" open previously saved data and plot it. Convert raw 12-bit ADC data to voltage """
	
	filename = "TestData-2015-05-15-1306.pkl"
	SR = SCR.SerialDataLogger()	
	t, C1, C2, C3, C4, C5, C6 = SR.load_data(filename)
	
	fig = plt.figure("Test load plot",figsize=(5,12))
	plt.clf()
	
	ax1 = fig.add_subplot(611)
	ax2 = fig.add_subplot(612,sharex=ax1)
	ax3 = fig.add_subplot(613,sharex=ax1)
	ax4 = fig.add_subplot(614,sharex=ax1)
	ax5 = fig.add_subplot(615,sharex=ax1)
	ax6 = fig.add_subplot(616,sharex=ax1)
	
	ax1.plot(t,C1*3.3/4095)
	ax2.plot(t,C2*3.3/4095)
	ax3.plot(t,C3*3.3/4095)
	ax4.plot(t,C4*3.3/4095)
	ax5.plot(t,C5*3.3/4095)
	ax6.plot(t,C6*3.3/4095)
	
	ax1.set_xlim(t[0],t[-1])
	
	for ax in [ax1,ax2,ax3,ax4,ax5,ax6]:
		ax.set_ylim(0,3.3)
		
	
	ax6.set_xlabel('Time (ms)')
	
	ax1.set_ylabel('A0 (V)')
	ax2.set_ylabel('A1 (V)')
	ax3.set_ylabel('A2 (V)')
	ax4.set_ylabel('A3 (V)')
	ax5.set_ylabel('A4 (V)')
	ax6.set_ylabel('A5 (V)')
	
	fig.tight_layout()
	
if __name__ == '__main__':
	main()