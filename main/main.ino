//#include <LedControl.h> // to use 8 digit segment display on arduino

#include "flow_sensor.hpp"
#include "timer.hpp"


// ################################################################################################
// ####################################### USER DEFINITIONS #######################################
// ################################################################################################

// ------- PIN DEFINITIONS -------
#define FLOW_SENSOR_PIN 2		    // Digital input pin for flow sensor readings
#define RED_LED_PIN 7           // Digital output pin for RED LED 
#define GREEN_LED_PIN 5         // Digital output pin for GREEN LED 
#define BUZZER_PIN 6         	  // PWM output pin for buzzer
#define RELAY_PIN 11			      // Digital output pin for relay
//#define MAX_DIN_PIN 12        // DIN pin for MAX segment display
//#define MAX_CLK_PIN 13        // CLK pin for MAX segment display
//#define MAX_CS_PIN 10         // CS pin for MAX segment display

// ------- CONSTANT DEFINITIONS -------
// Define sensor calibration constant for the hall-effect flow sensor.
// In my case the flow sensor is YF-B7. It outputs approximately 11.0 pulses per second per liter/minute of flow.
// In other words, if the number of pulses read from the pin during 1 second is 11.0, then the measured flow rate is 1L/min
// (ref. https://www.tinytronics.nl/shop/en/sensors/liquid/yf-b7-water-flow-sensor-with-temperature-sensor-brass-g1-2 )
const double SENSOR_CALIBRATION_FACTOR = 11.0;

// time lenght transitions between states
const unsigned long MAX_WATER_TIME = 270;     // maximum time allowed for the water to be running, in seconds
const unsigned long MAX_BATH_LENGTH = 450;    // maximum time allowed for the bath, in seconds
const unsigned long WARN_TIME = 30;           // time (until end of bath start) to warn end of bath, in seconds
const unsigned long RECOVERY_TIME = 5;        // time of recovery mode, in minutes
const unsigned long TRANSITION_TIME = 30;     // time to transition from IDLE mode to BATH mode, in seconds
const double        FLOW_THRESHOLD = 0.0;     // flow-rate threshold to trigger transition to BATH states

// ################################################################################################
// ################################### END OF USER DEFINITIONS ####################################
// ################################################################################################

#define SEC2MILLIS  1000                      // conversion from seconds to milliseconds (1000)
#define MIN2MILLIS  60000                     // conversion from minutes to milliseconds (1000 * 60)

// Relay is configured in High level trigger - normally open mode 
// see https://arduinogetstarted.com/tutorials/arduino-relay for details
// To use this mode, we connect the high voltage device to the COM pin and NO pin
//  * if the IN pin is connected to LOW (0V), the switch is open. The device is off.
//  * if the IN pin is connected to HIGH (5V), the switch is closed. The device is on.

enum State {IDLE, BATH, RECOVERY};

// control variables
State state;
// LedControl led_controller = LedControl(MAX_DIN_PIN,MAX_CLK_PIN,MAX_CS_PIN,1);
FlowSensor flow_sensor{SENSOR_CALIBRATION_FACTOR};

bool init_state; // execute initial conditions when a new state is triggered
unsigned long bath_count {0}; // number of baths counter
bool transition_idle_bath = false; // boolean to check transition from IDLE to BATH

// time variables
bool timer_on = false; // activate or deactivate timer
bool water_on = false; // for soap time logic

unsigned long sensor_time {0};         // variable used to store sensor timetags
unsigned long bath_time_counter {0};   // variable to count time taken by the current bath
unsigned long soap_time {0};           // store time stamps for soap time logic
unsigned long state_time {0};          // variable used to store state timetags
unsigned long rollover_time{0};        // store timestamp in loop() function and check for rollover

/*flash() function is called when timer interrupt is triggered*/
void flash()
{
    // flash the LED and BUZZER
    static boolean output = HIGH;
    digitalWrite(BUZZER_PIN, output);
    digitalWrite(GREEN_LED_PIN, output);
    output = !output;
}

void setup() 
{
    // connect LEDs, BUZZER and RELAY
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    
    // Water flow sensor configured to trigger on a FALLING state change (transition from HIGH state to LOW state)
    pinMode(FLOW_SENSOR_PIN, INPUT);
    digitalWrite(FLOW_SENSOR_PIN, HIGH);
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowSensorInterrupt, FALLING);
    
    // define initial state 
    state = State::IDLE;
    init_state = true;
    
    // initialize digit segment display
    //led_controller.shutdown(0,false);
    //led_controller.setIntensity(0,5);
    //led_controller.clearDisplay(0);

    // Timer1 setup
    timerSetup();

    rollover_time = millis();    
}

void flowSensorInterrupt()
{
    flow_sensor.pulseCounter();
}

void stateTransition()
{
    if (state == State::IDLE)
    {
  		  state = State::BATH;
    }
  	else if (state == State::BATH)
    {
  		  state = State::RECOVERY;
    }
  	else // if state == State::RECOVERY
    {
  		  state = State::IDLE;
    }
  	init_state = true;
}

void measureFlowRate()
{
  	if((millis() - sensor_time) > 1000.0)    // measure flow rate at frequency of 1Hz
  	{
    		// Disable the interrupt while calculating flow rate and sending the value to the host
    		detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));
    
    		// compute flow rate
    		flow_sensor.computeFlowRate(millis(), sensor_time);
    
    		// Print the flow rate in [L/min], in BATH state (or transition to bath)
        //if (state == State::BATH || transition_idle_bath)
        //{
        //    writeFlowToLCD(led_controller, flow_sensor.getFlow());
        //}

        // Reset the pulse counter, so we can start incrementing again
        flow_sensor.resetCounter();
        
    		// Enable the interrupt again now that we've finished sending output
    		attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowSensorInterrupt, FALLING);
    
    		// update time variable
    		sensor_time = millis();
  	}	 
}

void loopIdle()
{
  	if (init_state)
  	{
    		// initial conditions for IDLE state
        digitalWrite(RED_LED_PIN, LOW); // turn off RED LED
        digitalWrite(GREEN_LED_PIN, LOW); // turn off GREEN LED
        digitalWrite(RELAY_PIN, LOW); // Set LOW to open valve (let water through)
        transition_idle_bath = false;
    		init_state = false;
        // writeCounterToLCD(led_controller, bath_count);
        disableTimer();
  	}

    // check if there is active flow (to start transition to BATH)
    if (!transition_idle_bath && flow_sensor.getFlow() > FLOW_THRESHOLD)
    {
        state_time = millis();
        transition_idle_bath = true;  
    }

    // check flow rate again, after TRANSITION_TIME has finished
    if (transition_idle_bath && (millis() - state_time >  TRANSITION_TIME * SEC2MILLIS))
    {
        if (flow_sensor.getFlow() > FLOW_THRESHOLD)
        {
            stateTransition();
        }
        else 
        {
            transition_idle_bath = false;
        }
    }
}

void loopBath()
{
  	if (init_state)
  	{
        //Serial.println("Setting initial conditions for BATH state");
    		// initial conditions for BATH state
        state_time = millis();
        digitalWrite(GREEN_LED_PIN, HIGH); // turn on GREEN LED
    		init_state = false;
        bath_time_counter = 0.0;
        water_on = true;
        timer_on = false;
        soap_time = state_time;
  	}

    // check if we are getting close to end of bath -> enable timer if true
    if (!timer_on && (millis() - state_time > (MAX_BATH_LENGTH * SEC2MILLIS - WARN_TIME * SEC2MILLIS)))
    {
         timer_on = true;
         enableTimer();
    }
    if (!timer_on && water_on && (millis() - soap_time + bath_time_counter  > (MAX_WATER_TIME * SEC2MILLIS - WARN_TIME * SEC2MILLIS)))
    {
        timer_on = true;
        enableTimer();
    }

    // if water is turned off allow for soaping period (extra time)
    if (water_on && flow_sensor.getFlow() <= FLOW_THRESHOLD)
    {
        water_on = false;
        bath_time_counter += millis() - soap_time;
    }
    else if (!water_on && flow_sensor.getFlow() > FLOW_THRESHOLD)
    {
        water_on = true;
        soap_time = millis();
    }
    
    // end of bath conditions
    if (millis() - state_time > MAX_BATH_LENGTH * SEC2MILLIS)
    {
        stateTransition();
    }

    if (water_on && (millis() - soap_time + bath_time_counter  > MAX_WATER_TIME * SEC2MILLIS))
    {
        stateTransition();
    }
}

void loopRecovery()
{
  	if (init_state)
  	{
        // initial conditions for RECOVERY state
        disableTimer();
        state_time = millis();
        digitalWrite(GREEN_LED_PIN, LOW); // turn off GREEN LED
        digitalWrite(RED_LED_PIN, HIGH); // turn on RED LED
        digitalWrite(BUZZER_PIN, LOW); // turn off BUZZER
        digitalWrite(RELAY_PIN, HIGH); // Set HIGH to close valve (do not let water through)
        bath_count ++; // increase bath counter
    		init_state = false;
        //led_controller.clearDisplay(0);
  	}

    if (millis() - state_time > RECOVERY_TIME * MIN2MILLIS)
        stateTransition();
}

void loop() 
{
  	if (state == State::IDLE)
  		  loopIdle();
  	else if (state == State::BATH)
  		  loopBath();
  	else // if state == State::RECOVERY
  		  loopRecovery();
  
  	// measure flow rate
  	measureFlowRate();

    // check for timer rollover
    if (rollover_time > millis())
    {
        // if we get here, then millis() has reset!
        state = State::RECOVERY;
        stateTransition();
    }
    else
    {
        // update rollover_time
        rollover_time = millis();
    }

    delay(100); // delay 100 ms
}
