/*
 * This is a program to detect power supply voltage by using the on chip 
 * analog-to-digital converter to measure the voltage of the internal band-gap reference voltage. 
 *
 * More info at...
 *
 * http://wp.josh.com/2014/11/06/battery-fuel-guage-with-zero-parts-and-zero-pins-on-avr/
 *
 */

// Returns the current Vcc voltage as a fixed point number with 1 implied decimal places, i.e.
// 40 = 4 volts, 35 = 3.5 volts,  29 = 2.9 volts
//
// On each reading we: enable the ADC, take the measurement, and then disable the ADC for power savings.
// This takes >1ms because the internal reference voltage must stabilize each time the ADC is enabled.
// For faster readings, you could initialize once, and then take multiple fast readings, just make sure to
// disable the ADC before going to sleep so you don't waste power.

// NOTE: THIS WORKS ON THE LEVEL OF THE BOARD STABILIZER (NOT THE BATTERY LEVEL)
// Since the stabilizer always shows 3,3V this code does not work :(
// When I reduce external voltage the external reset circuit resets the device before this code
// detects the low voltage. Hence it is useless.
// We should rework the board to take directly the Vcc from the battery in order this to work.

#include "battery.h"

unsigned char readVccVoltage(void)
{
  //http://www.gammon.com.au/adc

  // Adjust this value to your boards specific internal BG voltage x1000
  const long InternalReferenceVoltage = 1100L; // <-- change this for your ATMEga328P pin 21 AREF value

  // REFS1 REFS0          --> 0 1, AVcc internal ref. -Selects AVcc external reference
  // MUX3 MUX2 MUX1 MUX0  --> 1110 1.1V (VBG)         -Selects channel 14, bandgap voltage, to measure
  ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0);

  // Let mux settle a little to get a more stable A/D conversion
  delay(50);

  // Start a conversion
  ADCSRA |= _BV(ADSC);

  // Wait for conversion to complete
  while (((ADCSRA & (1 << ADSC)) != 0))
    ;

  // Scale the value - calculates for straight line value
  unsigned int results = (unsigned int)((InternalReferenceVoltage * 1024L) / ADC) / 100L;
  return results;
}

bool isVoltageLow() {
  unsigned char vcc = readVccVoltage();
  return vcc < 32 ? true : false;
}