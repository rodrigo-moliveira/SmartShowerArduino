#ifndef TIMER_H
#define TIMER_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <LedControl.h>

// Timer1 functions
void flash();

// Timer1 functions
void timerSetup();
void disableTimer();
void enableTimer();
ISR(TIMER1_COMPA_vect);

// LCD helper functions
void writeCounterToLCD(LedControl& led_controller, unsigned long x);
void writeFlowToLCD(LedControl& led_controller, double x_);

#endif // TIMER_H
