#include <Arduino.h>

#include <avr/sleep.h>
#include <avr/wdt.h>

#define INIT_SLEEP_TIME 1 // Required to start up the device in sleep mode for a short time
#define SLEEP_TIME 1 // Normal sleep time (1 is 8 seconds, which is the max of watchdog, other numbers is x times 8 seconds)

void initSleep(void);
void goToSleep(char);