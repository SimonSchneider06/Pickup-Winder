/*
  author: Simon Schneider, apollonguitars.de
  version: 0.1.2
  date: 1st April 2025

  board: arduino uno

  helpful ressources: https://forum.arduino.cc/t/get-rpm-output-from-bldc-8015a-motor-driver/1326077/2 

*/

//std C libs
#include <math.h>

//arduino libs


//own libs
#include "config.h"

//== var declaration ==

// inputs
int SecurityStopIn;
int EndStopIn;
int StartProgram;

// x-axis movement
unsigned long stepsPerRotation;
unsigned long posX;
volatile int pulses;    // of the winding motor, to calculate the RPM

// for the timing of the pulses for the x-axis
// need to be global, so they can be remembered for the next cycle
int lastMotorPulse;
unsigned long currentTime;
unsigned long lastTime;

/* For measuring the RPM Pulses 

  unsigned long currentTimeRPM;
  unsigned long lastTimeRPM;
  unsigned long sek60;
*/
unsigned long steps;
unsigned long stepsAtOnce;
double usekPerPulseX;

int distanceX;    // the distance for the x-axis to move
double XAxisRows; // the number of rows the x-axis has to move

// wind motor
int writeWindRPM;
int windOutSpeed;
int sigToRPM;     // the factor with which the hall sensor output has to be multiplied, to give the rpms

//function declaration
unsigned long calculateStepsToDistance(int distance);
double calculateStepSetup();
int moveStepperPerTimeStep();
void securityStop();
void countRPM();


void setup() {
  
  //define Pins
  pinMode(WIND_ROTATION_DIRECTION_PIN, OUTPUT);
  pinMode(WIND_AVI_CONTROL_PIN, OUTPUT);
  pinMode(WIND_SPEED_OUT_PIN, INPUT_PULLUP);

  pinMode(STEP_DIRECTION_PIN, OUTPUT);
  pinMode(STEP_PULSE_PIN, OUTPUT);
  pinMode(STEP_ENABLE_PIN, OUTPUT);

  pinMode(END_STOP_INPUT_PIN,INPUT);
  pinMode(START_PROGRAM_PIN, INPUT);

  // interrupt for security stop
  attachInterrupt(digitalPinToInterrupt(SECURITY_STOP_PIN),securityStop,RISING);

  /* For Measuring the RPM Pulses

    attachInterrupt(digitalPinToInterrupt(WIND_SPEED_OUT_PIN),countRPM,RISING);
    lastTimeRPM = millis();
    sek60 = 60000;
  */

  // unclear if enable pin is needed or not connected, documentation is not clear
  // machine works this way...
  digitalWrite(STEP_ENABLE_PIN, LOW); //so x-axis can move


  usekPerPulseX = ceil(calculateStepSetup());       //calculate PulseTime for X-Axis, round up, so no error
  XAxisRows = round(calculateXAxisRowCount());      // only does full rows, rounds RowCount
  distanceX = PICKUP_WINDING_HEIGHT;  // distance the Motor has to move per Row

  // start turing motor with specific rpm
  writeWindRPM = map(WIND_RPM,0,4728,0,255);

  bool start = false;

  Serial.begin(57600);

  // move Initialisation messages in separate function
  // TODO
  Serial.print("\n==Start of Programm==\n");
  Serial.print("RPM:\t");
  Serial.print(WIND_RPM);
  Serial.print("\tNumber of Windings:\t");
  Serial.print(WINDING_NUMB);
  Serial.print("\tNumber of Rows:\t");
  Serial.print(XAxisRows);
  Serial.print("\tPulse Time:\t");
  Serial.print(usekPerPulseX);
  Serial.print(" us\n");
  

  SecurityStopIn = 0; // initialize security stop

  moveToStart();    // startup program

  // == Start Logic ==
  Serial.println("Press Start Button to Start Winding");

  // wait to press start button
  do{
    StartProgram = digitalRead(START_PROGRAM_PIN);
    delay(50);    // 50ms
  }
  while(StartProgram == LOW);

  Serial.println("Starting winding...");

  // Start Program

  //set direction of x-axis motor
  // LOW = x-axis towards the motor, HIGH = x-axis away from the motor
  digitalWrite(STEP_DIRECTION_PIN,HIGH);
  delay(5);   // 5ms

  digitalWrite(WIND_ROTATION_DIRECTION_PIN,HIGH);   // set direction of winding motor. HIGH = clockwise
  analogWrite(WIND_AVI_CONTROL_PIN, writeWindRPM);  // set Motor speed
}

void loop() {

  // loops the amount of rows
  if(XAxisRows > 0){

    posX = moveDistance(posX,distanceX);    // move forward or backwards, depending
    distanceX = distanceX * (-1);           // toggles direction
    //Serial.print(posX);
    XAxisRows--;
  }
  // finished all rows, deactivate wind motor
  else{
    analogWrite(WIND_AVI_CONTROL_PIN, 0);
    Serial.println("Completed Winding.\n\n");
    delay(5000);
    exit(0);
  }

}


/**
  * Stops the program. 
  * Called through an interrupt by pressing the 
  * "security stop button"
  *
*/
void securityStop(){

  // shows that the programm went into emergency stop mode
  analogWrite(WIND_AVI_CONTROL_PIN, 0); //switch wind motor off
  Serial.print("\n Security Stop\n");
  delay(5000);

  exit(1);  // some random error code
}


/* For measuring the RPM Pulses

  void countRPM(){

    pulses++;

    currentTime2 = millis(); // get ms

    if((currentTime2 - lastTime2) >= sek60){
      Serial.print("Rotations: ");
      Serial.print(pulses/WIND_PULSES_PER_ROTATION);
      Serial.print("\n\n");
      lastTime2 = currentTime2;
    }

  }
*/


/**
  * Calculates the duration of one on-off-cycle, so the x-axis moves with the exact speed to ensure 1 wire per rotation
  * Based on the rpm of the winding.
  *
*/
double calculateStepSetup(){

  /* Formulas:

    double Rot_X_Per_Wire = WIRE_DIAMETER / SPINDLE_PITCH;
    double Rot_W_Per_usek = WIND_RPM / (60.0*1000*1000);  //calculate in us
    double Wire_Per_Rot_W = 1;
    double Wire_Per_usek = Wire_Per_Rot_W*Rot_W_Per_usek;
    double usek_Per_Rot_X = 1.0/(Rot_X_Per_Wire*Wire_Per_usek);
    double usek_Per_Pulse = usek_Per_Rot_X / STEP_MOTOR_DRIVER_PULSE_PER_REV;

    --> Writtten in one line, to prevent rounding errors
    usek_Per_Pulse = (SPINDLE_PITCH * usek) / (WIRE_DIAMETER * Wire_Per_Rot_W * RPM * STEP_MOTOR_DRIVER_PULSE_PER_REV)
    units:           (um/RotX  * usek/min ) / (um/Wire        * Wire/RotW     * RotW/min    * Pulse/Rev)
  */

  double Wire_Per_Rot_W = 1.0;
  double usek_Per_Pulse_Cycle = (SPINDLE_PITCH * 60.0*1000*1000) / (WIRE_DIAMETER * Wire_Per_Rot_W * WIND_RPM * STEP_MOTOR_DRIVER_PULSE_PER_REV);
  double usek_Per_Pulse_Change = usek_Per_Pulse_Cycle/2;  // needs to be halved, because the time is used to trigger high and low state, and one h-l cycle is one pulse

  //TODO if its below min, throw error
  if(usek_Per_Pulse_Change < STEP_DRIVER_MIN_USEK_PER_PULSE_CHANGE){
    return 0;
  }

  return usek_Per_Pulse_Change; 

}


/**
 * Calculates the number of rows the x-axis has to drive in order to get the desired coil windings
 * 
 * The number of turns for the x-axis is the row number minus 1, because for 2 rows the machine needs to 
 * turn once, for 3 rows twice, etc.
 *
 * @return the number of rows
*/
double calculateXAxisRowCount(){
  /* Formulas:

    Number_of_Rows * Number_of_Winds_Per_Row = WINDING_NUMB
    Number_of_Winds_Per_Row = PICKUP_WINDING_HEIGHT / WIRE_DIAMETER

    eq. rearranged to solve: Number_of_Rows = WINDING_NUMB / Number_of_Winds_Per_Row

    --> written in one line to prevent rounding errors:
    Number_of_Rows = WINDING_NUMB * WIRE_DIAMETER / PICKUP_WINDING_HEIGHT
    units: (1)     =      (1)        *      um       /          um

  */

  return WINDING_NUMB * WIRE_DIAMETER / PICKUP_WINDING_HEIGHT;

}



/**
  * Calculates the number of Steps the Stepper Motor needs to do, in order for the x-axis to move that distance
  *
  * @param distance in um
  * @return the number of Steps
*/
unsigned long calculateStepsToDistance(int distance){

  //the distance the Wire guide moves when the stepper motor is doing one step (i.e. distance per pulse); in um
  unsigned long distancePerStep = SPINDLE_PITCH / STEP_MOTOR_DRIVER_PULSE_PER_REV;

  unsigned long stepNumber = round(distance / distancePerStep); //could be a floating point value -> round

  return stepNumber;
}


/** 
  * Moves the Linear Axis the desired distance
  *
  * @param currentPos is the current Position of the x-axis, in um
  * @param distance is the distance the x-axis should move, in um. If the distance is:
  *                 - positive, the x-axis moves to the right (away from the zero position)
  *                 - negative, the x-axis moves to the left (towards the zero position)
  * 
  * @return the new position
*/
unsigned long moveDistance(unsigned long currentPos, int distance){

  // calculate amount of pulses to send. for each step 2 pulses (one HIGH, one LOW needs to be sent)
  // without the "abs" the negative distance gives negative steps, but since "steps" is of type unsigned its interpreted as an gigantic amount of positive steps
  unsigned long steps = calculateStepsToDistance(abs(distance));  
  unsigned long pulses = steps * 2;

  // set direction
  setMoveDirection(distance);

  // loop to move x-axis
  // security stop through Interrupt
  while(pulses > 0){
    pulses -= moveStepperPerTimeStep();
  }

  // todo change pos in loop; by security stop its different!
  return currentPos += distance;  //return new position

}


/** 
  * Sets the Direction of the Motor of the X-Axis
  *
  * @param distance if it's > 0, move right, < 0 move left
  * 
  * @return None
*/
void setMoveDirection(int distance){

  if(distance > 0){
    digitalWrite(STEP_DIRECTION_PIN, HIGH); // x-axis moves away from motor (right)
  }
  else{
    digitalWrite(STEP_DIRECTION_PIN, LOW); // x-axis moves towards motor (left)
  }

}


/** 
  * Writes one pulse, in the right time intervall to the motor.
  * It's either a HIGH-Pulse or a LOW-Pulse
  * For one step, the arduino needs to send 2 pulses
  * 
  * @return 1, if a pulse got written, 0 if no pulse got written.
*/
int moveStepperPerTimeStep(){
  //for the protocol look inside Documentation: DM542T p.9
  // returns 1, if sent one pulse (2 pulses (HIGH & LOW) for 1 step)
  
  // var definition
  

  currentTime = micros(); // get us

    // check if enought time passed, to change the pulse
    // usekPerPulseX is calculated using the function "calculateStepSetup"
    if(currentTime - lastTime >= usekPerPulseX){

      if(lastMotorPulse == HIGH){
        lastMotorPulse = LOW;
      }
      else{
        lastMotorPulse = HIGH;
      }
      digitalWrite(STEP_PULSE_PIN,lastMotorPulse);
      //Serial.println("Pulse");

      lastTime = currentTime;
      return 1;
    }
  
  return 0;
}


/** 
  * Homes the machine, as it goes to machine 0, which is the position, where the end-stop-triggers
  * 
  * @return None
*/
void goToZero(){
  // move towards motor, until end stop sensor is activated

  // status update
  Serial.println("\n\nMachine is Homing...\n");

  // set direction of x-axis towards the motor (HIGH = away)
  digitalWrite(STEP_DIRECTION_PIN,LOW); 
  EndStopIn = 0;

  // endstop is in interrupt handler
  while(!EndStopIn){

    EndStopIn = digitalRead(END_STOP_INPUT_PIN);
    moveStepperPerTimeStep();

  }

  // set position to 0
  posX = 0;

  // status update
  Serial.println("Machine Homed.\n");
}


/** 
  * Moves the machine to the start position, i.e. the position at the pick-up to start winding
  * Homes first the machine and then goes to the specified offset of 
  * "PICKUP_BOTTOM_PLATE_THICKNESS + PLATE_OFFSET" (see config.h for more information)
  * 
  * 
  * @return None
*/
void moveToStart(){
  
  // first home x-axis
  goToZero();

  // status update
  Serial.println("Moving to start position of Pickup...\n");

  // get start position
  int startPos = PICKUP_BOTTOM_PLATE_THICKNESS + PLATE_OFFSET;
  
  // move to start position
  posX = moveDistance(0, startPos);

  Serial.print("Moved To Startposition: x = ");
  Serial.print(posX);
  Serial.print("\n");

}


