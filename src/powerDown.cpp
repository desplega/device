/*------------------------------------------------------------------------------ 
Libraries:    
              - Arduino.h (to define ISR)
              - avr/sleep.h (included w Arduino IDE); 
              - avr/wdt.h (included w Arduino IDE); 
References: 
              - Atmel ATmega328-328P_Datasheet:
			          http://www.atmel.com/Images/Atmel-42735-8-bit-AVR-Microcontroller-ATmega328-328P_Datasheet.pdf
Date:         07/11/2019
Author:       desplega (www.desplega.com)
------------------------------------------------------------------------------*/

#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "powerDown.h"

volatile char wdt_counter = 0; // Counter for Watchdog

// Watchdog interrupt
// Note: usually <Arduino.h> is required to use ISR
ISR(WDT_vect)
{
  // This is the watchdog interrupt function called after waking up
  wdt_counter++;
}

// Init sleep mode
void initSleep()
{
  // Setup watchdog
  WDTCSR = bit(WDCE) | bit(WDE);
  // Set interrupt mode and an interval
  WDTCSR = bit(WDIE) | bit(WDP3) | bit(WDP0); // Set WDIE, and 8 seconds delay
  wdt_reset();                                // Reset the watchdog

  // Disable ADC
  ADCSRA = 0;

  //ENABLE SLEEP - this enables the sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // set up sleep mode
  sleep_enable();
}

void goToSleep(char time)
{
  wdt_counter = 0;
  wdt_reset(); // Reset the watchdog
  do
  {
    //BOD disable - this must be called right before the __asm__ sleep instruction
    MCUCR = bit(BODS) | bit(BODSE);
    MCUCR = bit(BODS);
    sleep_mode(); // Entering sleep mode
  } while (wdt_counter < time);
}
