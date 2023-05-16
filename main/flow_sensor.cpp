#include "flow_sensor.hpp"

FlowSensor::FlowSensor(double calibration_factor):
calibration_factor{calibration_factor}, pulse_count{0}, flow_rate{0.0}, flow{0.0}
{
}

void FlowSensor::pulseCounter()
{
    // Increment the pulse counter
    pulse_count++;
}
    
void FlowSensor::computeFlowRate(unsigned long current_time, unsigned long start_time)
{
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flow_rate = ((1000.0 / (current_time - start_time)) * pulse_count) / calibration_factor; // flow rate in L/min
    
    // compute flow from flow_rate, in mL
    flow = (flow_rate / 60.0) * 1000.0;
    
    // Add the millilitres passed in this second to the cumulative total
    // totalMilliLitres += flowMilliLitres;
}

void FlowSensor::resetCounter()
{
    pulse_count = 0;
}

double FlowSensor::getRate()
{
    return flow_rate;
}

unsigned long FlowSensor::getFlow()
{
    return flow;
}
