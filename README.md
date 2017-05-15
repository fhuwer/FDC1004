# FDC1004
C-Library for use on AVR microcontrollers to setup and readout an TI FDC1004
capacity sensor.  The I2C-communication is done using the i2cmaster library
written by Peter Fleury.

Currently the library supports all the basic measurements and is easy to use.

The following code sample can be used to do measurements:

        
    #include <FDC1004.h>
    
    int main(void) {
       // Intialize FDC1004
       if (FDC1004_init()) {
          // Initialization failed, do something about it here.
       }

       double result;
       while (1) {
          FDC1004_measure_channel(0, &result);
          // Do something with the measuring result, for example send it to your PC via serial port.

          // Wait some time before next measurement.
          _delay_ms(100);
       }
    }
