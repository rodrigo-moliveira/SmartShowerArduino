#ifndef FLOW_SENSOR_H
#define FLOW_SENSOR_H

#include <stdint.h>

typedef uint8_t byte;

class FlowSensor
{

    /*
    Description of Hall-effect flow sensor:

    In my case the selected sensor was YF-B7 (https://www.seeedstudio.com/Water-Flow-Sensor-YF-B7-p-2884.html)
    
    The sensor outputs 11 pulses per second with a duty cycle of approximately 50% for every liter of liquid
    that passes through it per minute. The formula is:
        Q [L/min] = f_{pulse} [Hz] / 11.0
    where Q is the measured flow in units of L/min, and f_{pulse} is pulse frequency output by the sensor.

    The sensor works at 1Hz, in the sense that we have to count the number of pulses for each second, 
    and then apply the convertion from pulse count to flow. This is performed with a period of 1 second
    */
    
    public:
        FlowSensor(double calibration_factor);
        void pulseCounter();
        void computeFlowRate(unsigned long current_time, unsigned long start_time); 
        void resetCounter();

        double getRate();
        unsigned long getFlow();
        unsigned long getTotalFlow();
    private:

    // control variables
    double calibration_factor;
    byte  pulse_count;  

    // output rate
    double flow_rate;
    double flow;

};

#endif // FLOW_SENSOR_H
