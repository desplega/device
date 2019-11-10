#include <Arduino.h>

#include <avr/sleep.h>
#include <avr/wdt.h>

#define INIT_SLEEP_TIME 1 // Required to start up the device in sleep mode for a short time
#define SLEEP_TIME 8 // Normal sleep time (x times 8 seconds, which is the max of watchdog)

void initSleep(void);
void goToSleep(char);