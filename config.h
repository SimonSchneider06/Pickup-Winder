/*
  author: Simon Schneider, apollonguitars.de
  version: 0.1.2
  date: 2nd April 2026

  board: arduino uno

*/

/*
  ####################################
  ##### USER CALIBRATED SETTINGS #####
  ####################################

  These values need to be calibrated before every turning
*/

// ###### MEASUREMENT UNIT is MICROMETER #####
//all values are in mikrometer (10^-6 m) like WIRE_DIAMETER, ...
//reason: float calculations can make mistakes at some digit behind the decimal point ~4-6th, avoid this using smaller units


// these values depend on your wire and pickup dimensions
// for more information on where the values get measured, check the documentation
#define WIRE_DIAMETER 63.0  //um -> 0.063mm
#define PICKUP_BOTTOM_PLATE_THICKNESS 4500 //measured, in um, Its actually 4000, but leave some security room 
#define PICKUP_WINDING_HEIGHT 14000 //measured, in um, Its actually 15000 but leave some security room

#define WINDING_NUMB 5000 // number of how many windings the machine does
#define WIND_RPM 1500  // How fast the winding motor spins


/*
  ####################################
  ###### HARDWARE CONFIGURATION ######
  ####################################

  These values need to be configured when the machine hardware gets changed
*/

// ==== WIND ====
// is short for WINDING_MOTOR, it's the motor who rotates the pickup
#define WIND_ROTATION_DIRECTION_PIN 8   // HIGH = clockwise rotation, LOW = counterclockwise rotation
#define WIND_AVI_CONTROL_PIN 10
#define WIND_SPEED_OUT_PIN 3      // the driver gives the current speed to the arduino, needs to be an interrupt pin (either 2 or 3)

#define WIND_PULSES_PER_ROTATION 12 // the Pulses the hall sensors output per mechanical rotation (tested manually)
                                    //formula in documentatinon: rpm = 60 * (speedOut[Hz] / 6 * motor_poles)
                                    //see documentation from BLDC-8015A Driver

// ==== STEP ====
// is short for STEPPER_MOTOR, it's the motor who guides the wire
#define STEP_DIRECTION_PIN 12
#define STEP_PULSE_PIN 11
#define STEP_ENABLE_PIN 13  // needs to be low, in order for the motor to turn
#define STEP_MAX_RPM 200 //woher?
#define STEP_MOTOR_DRIVER_PULSE_PER_REV 1600.0     // Steps for one full rotation, selected by driver switches
#define STEP_DRIVER_MIN_USEK_PER_PULSE_CHANGE 50   // min amount of us pause between the change from low to high or high to low

#define START_PROGRAM_PIN 5
#define END_STOP_INPUT_PIN 4
#define SECURITY_STOP_PIN 2


// ==== MECHANICAL VALUES ====
//the distance the x-axis moves by one rotation
#define SPINDLE_PITCH 5000  // in um, result form calibration

#define SPINDLE_LENGTH 150000 // in um (=15cm length), needed to stop the motor before going

//how far the surface of the actual turning plate - where the pickup is mounted - is away from the zero position of the x-axis
#define PLATE_OFFSET 3000 // the position from absolute 0 to where the plate is



/*
  ###################################
  ######### X-AXIS PROTOCOL #########
  ###################################
*/

#define MIN_PULSE_WIDTH_DURATION 2.5 // in us
#define MIN_LOW_LEVEL_WIDTH_DURATION 2.5 // in us


