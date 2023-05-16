#include "timer.hpp"

void timerSetup()
{
    // set up timer1 interrupts with 1 Hz frequency
    cli();
    TCCR1A = 0;
    TCCR1B = 0; 
    OCR1A = 0xF424;
    TCCR1B = (1<<WGM12) | (1<<CS12); 
    TIMSK1 = 0; // start with timer disabled
    sei(); 
}

void disableTimer()
{
    TIMSK1 = 0;
}

void enableTimer()
{
    TIMSK1 = (1<<OCIE1A);     
}

ISR(TIMER1_COMPA_vect)
{
    flash();
}

void writeFlowToLCD(LedControl& led_controller, double x_)
{
    led_controller.clearDisplay(0);
    
    unsigned long x = (unsigned long) (x_ * 10000.0);
    
    led_controller.setDigit(0,7,x/10000000,false);
    x = x%10000000;
    led_controller.setDigit(0,6,x/1000000,false);
    x = x%1000000;
    led_controller.setDigit(0,5,x/100000,false);
    x = x%100000;
    led_controller.setDigit(0,4,x/10000,true);
    x = x%10000;
    led_controller.setDigit(0,3,x/1000,false);
    x = x%1000;
    led_controller.setDigit(0,2,x/100,false);
    x = x%100;
    led_controller.setDigit(0,1,x/10,false);
    x = x%10;
    led_controller.setDigit(0,0,x/1,false);
}

void writeCounterToLCD(LedControl& led_controller, unsigned long x)
{
    led_controller.clearDisplay(0);
        
    led_controller.setDigit(0,7,x/10000000,false);
    x = x%10000000;
    led_controller.setDigit(0,6,x/1000000,false);
    x = x%1000000;
    led_controller.setDigit(0,5,x/100000,false);
    x = x%100000;
    led_controller.setDigit(0,4,x/10000,false);
    x = x%10000;
    led_controller.setDigit(0,3,x/1000,false);
    x = x%1000;
    led_controller.setDigit(0,2,x/100,false);
    x = x%100;
    led_controller.setDigit(0,1,x/10,false);
    x = x%10;
    led_controller.setDigit(0,0,x/1,false);
}
