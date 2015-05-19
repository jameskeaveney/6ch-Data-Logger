/*

Six channel data logger using Arduino DUE.

**
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
**

Analog inputs on pins A0-A5 are read-in at a sampling rate of ~2 kS/s
The sampling rate can be increased up to around ~16 kS/s for 6 channels, probably faster for
fewer channels [channels must be enabled/disabled by setting the registers in the setup() method below],
by adjusting the sampling_time variable

The ADC data is buffered then sent quickly via SerialUSB. Since there is some overhead 
involved with Serial transfer, the data should be transferred in large quantities ( > 1 kB) so
as to minimise total transmission time. Buffering will be memory-limited at some point. The
largest buffer possible is currently untested!

PySerial is used to grab the data from the serial port and read it into python. For this,
the python module SixChannelReader.py has been written (in the same directory as this file), 
which provides class-based methods of reading in the data, as well as a method of 
synchronisation for triggering purposes.

************
** NOTE: the USB connection to the Arduino must be to the NATIVE port, not the programming port!
************



Author: James Keaveney
18/05/2015

*/

// set up variables
const int buffer_length = 200; // length of data chunks sent at a time via SerialUSB

const long acquisition_time = 90000; // in ms (~2 minutes in real experiment)
const long settling_time_ms = 15000; // in ms
const int sampling_time = 440; // in micros

const int TempAdjustPin = 2;

String strTime = "";
String strOut = "";
String strC1 = "";
String strC2 = "";
String strC3 = "";
String strC4 = "";
String strC5 = "";
String strC6 = "";


unsigned long current_time;

void setup() {
  // set up digital pin
  pinMode(TempAdjustPin, OUTPUT);
  
  // set ADC resolution
  analogReadResolution(12);
  
  // manually set registers for faster analog reading than the normal arduino methods
  ADC->ADC_MR |= 0xC0; // set free running mode (page 1333 of the Sam3X datasheet)
  ADC->ADC_CHER = 0xFC;  // enable channels  (see page 1338 of datasheet) on adc 7,6,5,4,3,2 (pin A0-A5)
                       // see also http://www.arduino.cc/en/Hacking/PinMappingSAM3X for the pin mapping between Arduino names and SAM3X pin names
  ADC->ADC_CHDR = 0xFF03; // disable all other channels
  ADC->ADC_CR=2;       // begin ADC conversion
  
  
  // initialise serial port
  SerialUSB.begin(115200); // baud rate is ignored for USB - always at 12 Mb/s
  while (!SerialUSB); // wait for USB serial port to be connected - wait for pc program to open the serial port
  
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
    
    // do something to alter the temperature - swap digital pin state
    digitalWrite(TempAdjustPin,!(digitalRead(TempAdjustPin)));
    
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
  
    // finally, print end-of-data and end-of-line character to signify no more data will be coming
    SerialUSB.println("\0");
    
    // wait a while to allow temperature to settle
    delay(settling_time_ms);
  }
}
