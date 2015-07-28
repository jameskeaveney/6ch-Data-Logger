/*

Six channel data logger using Arduino DUE.

**
Original purpose is as a slow (~kS/s) multi-channel data logger for use with a DFB laser, where 
a frequency sweep is controlled via temperature tuning. The on-board DAC is used, with an external
amplifier (gain adjustable), to alter the setpoint of a Thorlabs TED200C temperature controller. 
Since the temperature scan is slow (~30 seconds), the temperature controller PID can follow the setpoint
in nearly real-time. Since we need to record lots of channels, and the sampling rate isn't very high, 
a normal oscilloscope isn't the best for this job.

Input signals are from the main experiment vapour cell, main power monitor, reference cell, 
reference power monitor, Fabry-Perot etalon, and the temperature monitor (actual temperature, 
rather than setpoint).
**

Analog inputs on pins A0-A5 are read-in at an adjustable sampling rate of ~2 kS/s.
The sampling rate can be increased up to around ~16 kS/s for 6 channels, probably faster for
fewer channels [channels must be enabled/disabled by setting the registers in the setup() method below],
by adjusting the sampling_time variable

With the current PCB design, can have up to 8 input channels, which are all buffered as a simple
overvoltage protection for the ADC pins. There is no other OVP circuitry, so the op-amps will blow
if too much voltage is used!

The ADC data is buffered then sent quickly via SerialUSB. Since there is some overhead 
involved with Serial transfer, the data should be transferred in large quantities ( > 1 kB) so
as to minimise total transmission time. Buffering will be memory-limited at some point. The
largest buffer possible is currently untested!

PySerial is used to grab the data from the serial port and read it into python. For this,
the python module SixChannelReader.py has been written (in the same directory as this file), 
which provides class-based methods of reading in the data, as well as a method of 
synchronisation for triggering purposes.

************
** NOTE: the USB connection to the Arduino Due must be to the NATIVE port, not the programming port!
************



Author: James Keaveney
18/05/2015
updated 27/07/2015 - changed digital temperature control to analog temperature control with Timer ISR

*/

#include <DueTimer.h>

// set up variables
const int buffer_length = 200; // length of data chunks sent at a time via SerialUSB

const long acquisition_time = 30000; // in ms
const long settling_time_ms = 10000; // in ms
const int sampling_time = 440; // in micros (60 microsec per loop + this delay; e.g. 190 => 250micros total = 4 kS/s)

const int StatusPin = 52;

String strTime = "";
String strOut = "";
String strC1 = "";
String strC2 = "";
String strC3 = "";
String strC4 = "";
String strC5 = "";
String strC6 = "";

int dac_output = 0;

unsigned long current_time;

void setup() {
  // set up digital pin for status indicator - ON while acquiring data
  // this could be used for LED output, or as a TTL trigger for an external 'scope
  pinMode(StatusPin, OUTPUT);
  digitalWrite(StatusPin, 0);
  
  // set ADC and DAC resolution
  analogReadResolution(12);
  analogWriteResolution(12);
  
  // initialise DAC0 output and wait for temperature to settle
  dac_output = 0;
  analogWrite(DAC0,dac_output);
  dacc_write_conversion_data(DACC_INTERFACE, dac_output);


  // manually set registers for faster analog reading than the normal arduino methods
  ADC->ADC_MR |= 0xC0; // set free running mode (page 1333 of the Sam3X datasheet)
  ADC->ADC_CHER = 0xFC;  // enable channels  (see page 1338 of datasheet) on adc 7,6,5,4,3,2 (pin A0-A5)
                       // see also http://www.arduino.cc/en/Hacking/PinMappingSAM3X for the pin mapping between Arduino names and SAM3X pin names
  ADC->ADC_CHDR = 0xFF03; // disable all other channels
  ADC->ADC_CR=2;       // begin ADC conversion
  

  // initialise serial port
  SerialUSB.begin(115200); // baud rate is ignored for USB - always at 12 Mb/s
  while (!SerialUSB); // wait for USB serial port to be connected - wait for pc program to open the serial port

  // setup DAC increment period - set by acquisition time
  int timer_period;
  timer_period = acquisition_time*1000 / 4095;
  //SerialUSB.println(timer_period);
  Timer3.attachInterrupt(dac_increment);
  Timer3.setPeriod(timer_period);
  

  // allow temperature to settle first
  delay(settling_time_ms);  
}


void dac_increment() {
  // function called to increment the DAC output at regular intervals - used by timer ISR
  dac_output++;
  dacc_write_conversion_data(DACC_INTERFACE, dac_output);
}

void loop() {

  // data acquisition will start with a synchronisation step:
  // python should send a single byte of data, the arduino will send one back to sync timeouts
  int incoming = 0;
  if (SerialUSB.available() > 0) // polls whether anything is ready on the read buffer - nothing happens until there's something there
  {
    incoming = SerialUSB.read();
    // after data received, send the same back
    SerialUSB.println(incoming);
    
 
    //wait a little time for python to be ready to receive data - (not sure if this is really necessary)
    delay(50); // ms
    
    // do something to alter the temperature - start timer
    Timer3.start();
    
    // turn status/trigger on
    digitalWrite(StatusPin, 1);

    // measure start time - then acquire data for an amount of time set by the acquisition_time variable
    unsigned long start_micros = micros();
    unsigned long start_time = millis();
    while (millis() < (start_time+acquisition_time))
    {
        
      // generate and concatenate strings
      for (int jj = 0; jj < buffer_length; jj++)
      {
        
        // ADC acquisition
        
        // can put this in a small loop for some averaging if required - takes ~ 60 microsec per read/concatenate cycle
        while((ADC->ADC_ISR & 0xFC)!=0xFC); // wait for conversion to complete - see page 1345 of datasheet     
        
        // concatenate strings
        current_time = micros()-start_micros;
        strTime.concat(current_time); // time axis
        strTime.concat(',');     
        strC1.concat(ADC->ADC_CDR[7]); // read data from the channel data register
        strC1.concat(',');
        strC2.concat(ADC->ADC_CDR[6]);
        strC2.concat(',');
        strC3.concat(ADC->ADC_CDR[5]);
        strC3.concat(',');
        strC4.concat(ADC->ADC_CDR[4]);
        strC4.concat(',');
        strC5.concat(ADC->ADC_CDR[3]);
        strC5.concat(',');
        strC6.concat(ADC->ADC_CDR[2]);
        strC6.concat(',');
        
        
        //Serial.print(current_time);
        //Serial.print(",");
        
        delayMicroseconds(sampling_time); // limit sampling rate to something reasonable - a few kS/s
      }
      
      // send data via SerialUSB
      // perform a flush first to wait for the previous buffer to be sent, before overwriting it
      SerialUSB.flush();
      strOut = strTime + strC1 + strC2 + strC3 + strC4 + strC5 + strC6;  
      SerialUSB.print(strOut); // doesn't wait for write to complete before moving on
      
      
      // clear string data - re-initialise
      strTime = "";
      strC1 = "";
      strC2 = "";
      strC3 = "";
      strC4 = "";
      strC5 = "";
      strC6 = "";
    }
  
    // turn status/trigger off
    digitalWrite(StatusPin, 0);

    // stop timer
    Timer3.stop();
    
    // finally, print end-of-data and end-of-line character to signify no more data will be coming
    SerialUSB.println("\0");
    
    // reset temperature, and wait a while to allow temperature to settle
    delay(2000);
    dac_output = 0;
    dacc_write_conversion_data(DACC_INTERFACE, dac_output);
    delay(settling_time_ms);
  }
}
