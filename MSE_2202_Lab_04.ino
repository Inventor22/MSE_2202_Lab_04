/*
MSE 2202 Lab 4_Partial
Language: Arduino
Authors: Michael Naish and Eugen Porter
Date: 12/01/30
Rev 1 - Initial version
*/
#include <Servo.h>
#include <EEPROM.h>
#include <MsTimer2.h>
#include <CharliePlexing.h>
Servo servo_LeftMotor;
Servo servo_RightMotor;
Servo servo_ArmMotor;
Servo servo_GripMotor;
// Uncomment keywords to enable debugging output
//#define DEBUG_MODE_DISPLAY
//#define DEBUG_SUB_MODE_DISPLAY
//#define DEBUG_MOTORS
//#define DEBUG_ARM_CONTROL
//#define DEBUG_LINE_TRACKERS
//#define DEBUG_ENCODERS
//#define DEBUG_ULTRASONIC
//#define DEBUG_RANGE_FINDING
//#define DEBUG_LINE_TRACKER_CALIBRATION
//#define DEBUG_MOTOR_CALIBRATION
//#define DEBUG_POT
//#define DEBUG_PID_CONTROLLER

//port pin constants
const int ci_encoder_Pin_A[2] = {2,3};
const int ci_Grip_Motor = 4;
const int ci_Ultrasonic_Ping = 5; //input plug
const int ci_Ultrasonic_Data = 6; //output plug
const int ci_Left_Motor = 7;
const int ci_Right_Motor = 8;
const int ci_Arm_Motor = 9;
const int ci_Left_Line_Tracker = A0;
const int ci_Middle_Line_Tracker = A1;
const int ci_Right_Line_Tracker = A2;
const int ci_Motor_Speed_Pot = A3;
const int ci_EnableMotorsSwitch = A4;
const int ci_Arm_Length_Pot = A5;
const int ci_Left_Line_Tracker_LED = 1;
const int ci_Middle_Line_Tracker_LED = 2;
const int ci_Right_Line_Tracker_LED = 3;
const int ci_Indicator_LED = 5;
const int ci_Heartbeat_LED = 6;
//constants
// EEPROM addresses
const int ci_Right_Motor_Offset_Address_L = 0;
const int ci_Right_Motor_Offset_Address_H = 1;
const int ci_Left_Motor_Offset_Address_L = 2;
const int ci_Left_Motor_Offset_Address_H = 3;
const int ci_Left_Line_Tracker_Dark_Address_L = 4;
const int ci_Left_Line_Tracker_Dark_Address_H = 5;
const int ci_Left_Line_Tracker_Light_Address_L = 6;
const int ci_Left_Line_Tracker_Light_Address_H = 7;
const int ci_Middle_Line_Tracker_Dark_Address_L = 8;
const int ci_Middle_Line_Tracker_Dark_Address_H = 9;
const int ci_Middle_Line_Tracker_Light_Address_L = 10;
const int ci_Middle_Line_Tracker_Light_Address_H = 11;
const int ci_Right_Line_Tracker_Dark_Address_L = 12;
const int ci_Right_Line_Tracker_Dark_Address_H = 13;
const int ci_Right_Line_Tracker_Light_Address_L = 14;
const int ci_Right_Line_Tracker_Light_Address_H = 15;
const int ci_Left_Motor_Stop = 200; // 200 for brake mode; 1500 for stop
const int ci_Right_Motor_Stop = 200;
const int ci_Grip_Motor_Open = 176; // Experiment to determine appropriate value
const int ci_Grip_Motor_Zero = 90; // "
const int ci_Grip_Motor_Closed = 65; // "72
const int ci_Display_Time = 500;
const int ci_Num_Encoders = 2;
const int ci_Encoder_Steps_Per_Revolution = 90;
const int ci_Line_Tracker_Calibration_Interval = 100;
const int ci_Line_Tracker_Cal_Measures = 20;
const int ci_Motor_Calibration_Time = 5000;
const int ci_OffTrackCountLimit = 7;
//variables
byte b_LowByte;
byte b_HighByte;
unsigned long ul_Echo_Time;
unsigned int ui_Left_Line_Tracker_Data;
unsigned int ui_Middle_Line_Tracker_Data;
unsigned int ui_Right_Line_Tracker_Data;
unsigned int ui_Motors_Speed;
unsigned int ui_Left_Motor_Speed;
unsigned int ui_Right_Motor_Speed;
unsigned long ul_encoder_Pos[ci_Num_Encoders] = {0,0};
unsigned long ul_old_Encoder_Pos[ci_Num_Encoders] = {0,0};
unsigned long ul_3_Second_timer = 0;
unsigned long ul_Display_Time;
unsigned long ul_Calibration_Time;
unsigned int ui_Right_Motor_Offset = 0;
unsigned int ui_Left_Motor_Offset = 0;
unsigned int ui_Cal_Count;
unsigned int ui_Left_Line_Tracker_Dark;
unsigned int ui_Left_Line_Tracker_Light;
unsigned int ui_Middle_Line_Tracker_Dark;
unsigned int ui_Middle_Line_Tracker_Light;
unsigned int ui_Right_Line_Tracker_Dark;
unsigned int ui_Right_Line_Tracker_Light;
unsigned int ui_Line_Tracker_Tolerance;
unsigned int ui_Robot_State_Index = 5;
// 0123456789ABCDEF
unsigned int ui_Mode_Indicator[6] = {0x00, //B0000000000000000, //stop
    0x00FF, //B0000000011111111, //Run
    0x0F0F, //B0000111100001111, //Calibrate line tracker light level
    0x3333, //B0011001100110011, //Calibrate line tracker dark level
    0xAAAA, //B1010101010101010, //Calibrate motors
    0xFFFF}; //B1111111111111111};
unsigned int ui_Mode_Indicator_Index = 0;
//display Bits 0,1,2,3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
int iArray[16] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,65536};
int iArrayIndex = 0;
boolean bt_Heartbeat = true;
boolean bt_3_S_Time_Up = false;
boolean bt_Do_Once = false;
boolean bt_Cal_Initialized = false;


//template <class T>
//int EEPROM_writeAnything(int ee, const T& value)
//{
//    const byte* p = (const byte*)(const void*)&value;
//    int i;
//    for (i = 0; i < sizeof(value); i++)
//        EEPROM.write(ee++, *p++);
//    return i;
//}
//
//template <class T>
//int EEPROM_readAnything(int ee, T& value)
//{
//    byte* p = (byte*)(void*)&value;
//    int i;
//    for (i = 0; i < sizeof(value); i++)
//        *p++ = EEPROM.read(ee++);
//    return i;
//}

enum SensorPosition { LEFT, MIDDLE, RIGHT };
boolean sensorState[3];

template <class T>
struct DeltaVar {
    T now, prev;
    T rateOfChange() { return now - prev; }
};

template <class T, class U>
struct PidController {
    float kP, kI, kD; // tuning constants.
    const float iMin, iMax;
    float proportional, integral, derivative;
    int updateFrequency;

    T targetPos;
    DeltaVar<T> error;
    T (*p_getError)(U sensorState);

    PidController(float _kP, float _kI, float _kD, float _iMin, float _iMax, T _targetPos, T (*_p_getError)(U)) :
        kP(_kP), kI(_kI), kD(_kD), iMin(_iMin), iMax(_iMax), targetPos(_targetPos), p_getError(_p_getError) {
            error.now  = 0;
            error.prev = 0;
            updateFrequency = 5; // 200 Hz refresh rate
#ifdef DEBUG_PID_CONTROLLER
            Serial.print("kP: ");
            Serial.println(kP);
            Serial.print("kI: ");
            Serial.println(kI);
            Serial.print("kD: ");
            Serial.println(kD);
            Serial.print("iMin: ");
            Serial.println(iMin);
            Serial.print("iMax: ");
            Serial.println(iMax);
#endif
    }
    T getError(U sensorState) {
        return (*p_getError)(sensorState);
    }
    float update(T currentPos) {
        static uint32_t then = millis();
        if (millis() - then > updateFrequency) {
            error.now    =  targetPos - currentPos;
            proportional =  error.now;
            integral     += error.now;
            if (integral <  iMin) integral = iMin;
            if (integral >  iMax) integral = iMax;
            derivative   =  error.rateOfChange();
            error.prev   =  error.now;
        }
        return proportional*kP + integral*kI + derivative*kD;
    }
};

PidController<int8_t, uint8_t> *pid;

int8_t calcError(uint8_t sensorStates) {
    static uint8_t prevSensorStates;
    int8_t  error;
    switch (sensorStates) {
    case B000:
        if (prevSensorStates < B010)
            error =  3;
        else if (prevSensorStates > B010)
            error = -3;
        else
            error =  0;
        break;
    case B100: error = -2; break;
    case B110: error = -1; break;
    case B010: error =  0; break;
    case B011: error =  1; break;
    case B001: error =  2; break;
    case B111: error =  4; break;
    }
    if (sensorStates != B000) prevSensorStates = sensorStates;
    return error;
}


// ************************************************************************************************************************

#define MAX_SPEED_REV 1000
#define MAX_SPEED_FWD 2000

const int adr_kp = 20;
const int adr_ki = 30;
const int adr_kd = 40;

uint16_t  range  = 0;

void setup()
{
    Serial.begin(9600);
    float p, i ,d;
    //EEPROM_readAnything<float>(adr_kp, p);
    //EEPROM_readAnything<float>(adr_ki, i);
    //EEPROM_readAnything<float>(adr_kd, d);

    /* *** WORKING ***
    P = 140
    I = 2
    D = 260
    */

    pid = new PidController<int8_t, uint8_t>(
        140, // kP 195
        2,   // kI
        260,   // kD 152
       -20.0, // integral Min
        20.0, // integral Max
        0,   // target Position
        &calcError); // error function


    pinMode(13,OUTPUT);
    CharliePlexing::set(); // Set up Charlieplexing for LEDs
    // set up ultrasonic
    pinMode(ci_Ultrasonic_Ping, OUTPUT);
    pinMode(ci_Ultrasonic_Data, INPUT);
    // set up drive motors
    pinMode(ci_Left_Motor, OUTPUT);
    servo_LeftMotor.attach(ci_Left_Motor);
    pinMode(ci_Right_Motor, OUTPUT);
    servo_RightMotor.attach(ci_Right_Motor);
    // set up arm motors
    pinMode(ci_Arm_Motor, OUTPUT);
    servo_ArmMotor.attach(ci_Arm_Motor);
    pinMode(ci_Grip_Motor, OUTPUT);
    servo_GripMotor.attach(ci_Grip_Motor);
    servo_GripMotor.write(ci_Grip_Motor_Zero);
    pinMode(ci_Middle_Line_Tracker, INPUT);
    pinMode(ci_Left_Line_Tracker, INPUT);
    pinMode(ci_Right_Line_Tracker, INPUT);
    pinMode(ci_Motor_Speed_Pot, INPUT);
    pinMode(ci_EnableMotorsSwitch, INPUT);
    pinMode(ci_Arm_Length_Pot, INPUT);
    // read saved values from EEPROM
    b_LowByte = EEPROM.read(ci_Right_Motor_Offset_Address_L);
    b_HighByte = EEPROM.read(ci_Right_Motor_Offset_Address_H);
    ui_Right_Motor_Offset = word(b_HighByte, b_LowByte);
    b_LowByte = EEPROM.read(ci_Left_Motor_Offset_Address_L);
    b_HighByte = EEPROM.read(ci_Left_Motor_Offset_Address_H);
    ui_Left_Motor_Offset = word(b_HighByte, b_LowByte);
    b_LowByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_L);
    b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
    ui_Left_Line_Tracker_Dark = word(b_HighByte, b_LowByte);
    b_LowByte = EEPROM.read(ci_Left_Line_Tracker_Light_Address_L);
    b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
    ui_Left_Line_Tracker_Light = word(b_HighByte, b_LowByte);
    b_LowByte = EEPROM.read(ci_Middle_Line_Tracker_Dark_Address_L);
    b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
    ui_Middle_Line_Tracker_Dark = word(b_HighByte, b_LowByte);
    b_LowByte = EEPROM.read(ci_Middle_Line_Tracker_Light_Address_L);
    b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
    ui_Middle_Line_Tracker_Light = word(b_HighByte, b_LowByte);
    b_LowByte = EEPROM.read(ci_Right_Line_Tracker_Dark_Address_L);
    b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
    ui_Right_Line_Tracker_Dark = word(b_HighByte, b_LowByte);
    b_LowByte = EEPROM.read(ci_Right_Line_Tracker_Light_Address_L);
    b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
    ui_Right_Line_Tracker_Light = word(b_HighByte, b_LowByte);
    ui_Line_Tracker_Tolerance = 100;

#ifdef DEBUG_POT
    Serial.print("Pot: ");
    Serial.println(analogRead(ci_Arm_Length_Pot));
#endif
}

void loop()
{
    if((millis() - ul_3_Second_timer) > 3000)
    {
        bt_3_S_Time_Up = true;
    }
    //button debounce and mode selection
    if(!CharliePlexing::uc_ButtonImageOutput)
    {
        if(bt_Do_Once == false)
        {
            bt_Do_Once = true;
            ui_Robot_State_Index++;
            ui_Robot_State_Index = ui_Robot_State_Index & 7;
            ul_3_Second_timer = millis();
            bt_3_S_Time_Up = false;
            bt_Cal_Initialized = false;
        }
    }
    else
    {
        bt_Do_Once = LOW;
    }
    //modes
    switch(ui_Robot_State_Index)
    {
    case 0: //Robot stopped
        {
            readLineTrackers();
            Ping();
            servo_LeftMotor.writeMicroseconds(ci_Left_Motor_Stop);
            servo_RightMotor.writeMicroseconds(ci_Right_Motor_Stop);
            ui_Mode_Indicator_Index = 0;
            break;
        }
    case 1: //Robot Run after 3 seconds
        {
            if(bt_3_S_Time_Up)
            {
                // read pot to set top motor speed
                ui_Motors_Speed = analogRead(ci_Motor_Speed_Pot);
                ui_Motors_Speed = map(ui_Motors_Speed, 0, 1023, 1650, 1900); // motors stall below 1650; too fast above // apply motor offsets
                constrainMotorSpeeds(ui_Motors_Speed);
                // read levels from line tracking sensors
                readLineTrackers();
                // read encoder counts
                ul_encoder_Pos[0] = CharliePlexing::ul_LeftEncoder_Count;
                ul_encoder_Pos[1] = CharliePlexing::ul_RightEncoder_Count;
                for(int i=0; i < ci_Num_Encoders; i++)
                {
                    if(ul_encoder_Pos[i] != ul_old_Encoder_Pos[i])
                    {
# ifdef DEBUG_ENCODERS
                        Serial.print("Encoder ");
                        Serial.print(i);
                        Serial.print(": ");
                        Serial.println(ul_encoder_Pos[i], DEC);
# endif
                        ul_old_Encoder_Pos[i] = ul_encoder_Pos[i];
                    }
                }
                // Line tracking code here...

                //switch (stage) {
                //case FOLLOW_LINE:
                //case GRAB_FLAG:

                int8_t sensorStates = (sensorState[LEFT]   << 2) +
                    (sensorState[MIDDLE] << 1) +
                    sensorState[RIGHT];

                int8_t error = pid->getError(sensorStates);

                static byte subMode(1);

                Serial.print("SM: ");
                Serial.println(subMode);

                uint16_t range;  //Instant range
                uint16_t avgRange;

                switch (subMode) {
                case 1:
                    //Drive forward using PID
                    //END: all black
                    followLine(error);
                    static int errorCount = 0;
                    if (error == 4) {
                        errorCount++;
                    } else if (errorCount > 0) {
                        errorCount--;
                    }
                    if (errorCount > 5) {
                        subMode++;
                        OpenGrip(1);
                    }

                    break;
                case 2: {
                    //*****************************
                    // Drive forward on sonar
                    // Get within 6 cm of the box
                    //*****************************
                    //Drive forward on sonar
                    //Want to get within 4 cm of desired range

                    //uint16_t flagRange = getRange();
                    //if (flagRange < 70) {
                    //    subMode++;
                    //} else {
                    //    int speed = map(flagRange, 70, 450, 1500, MAX_SPEED_FWD);
                    //    ui_Left_Motor_Speed  = speed;
                    //    ui_Right_Motor_Speed = speed;
                    //}

                    static const int desiredRange(80);
                    
                    range    = getRange();  //Instant range
                    avgRange = getAvgRange(); //Range averaged over n cycles
                    
                    if((range<(desiredRange+60))&&(range>=(desiredRange+15))){
                      //Time to drive really slow (right at stall speed)
                      //Actually decelerating here
                      constrainMotorSpeeds(1620);            
                    }
                    else if(range<(desiredRange-5)){  //Back up
                        ui_Left_Motor_Speed = 1380;
                        ui_Right_Motor_Speed = 1380;
                    }
                    else if(range<(desiredRange+15)){  //If closer than 1.5 cm to the desired range, stop
                      ui_Left_Motor_Speed = ci_Left_Motor_Stop;
                      ui_Right_Motor_Speed = ci_Right_Motor_Stop;
                      //Wait for the arm to settle before 
                      static byte timeSettled(0);
                      if(SetReach(avgRange+150)){ 
                        if(!timeSettled){
                          timeSettled=millis();
                        }
                        else if((millis()-timeSettled)>200){ //Make sure that the arm is stable for 1/10 of a second before advancing
                           /* OpenGrip(0);
                            delay(800);*/
                            subMode++;
                        }
                      }
                      else {
                          timeSettled = 0;
                      }
                    }
                    break;
                        }
                case 3:
                    //**********************************
                    // Pick up flag as soon as the
                    // arm has settled on its position
                    // for a couple ms
                    //**********************************
                    
                    //Keep motors stopped
                    ui_Left_Motor_Speed  = ci_Left_Motor_Stop;
                    ui_Right_Motor_Speed = ci_Right_Motor_Stop;
                  
                    getRange();  //Must be called to se the current range for getAvgRange()
                    avgRange = getAvgRange();
                    if(SetReach(avgRange+150)){
                        static bool closing = false;
                        static unsigned long closeTime;
                        if(!closing){
                            OpenGrip(0);                 //Grab flag
                            closeTime = millis();
                            closing = true;
                        }
                        else if((millis()-closeTime)>350){
                            //Servo closed, time to go
                            SetReach(100); //Pull arm in as far as it goes
                            subMode++;
                        }
                    }
                    break;
                case 4:
                    //Reverse until at a nice distance
                    //Drive backward on sonar
                    uint16_t range;
                    SetReach(100);
                    if((range=getRange())<120){
                        // where it was before...
                        //Reverse motor speeds
                        //ui_Motors_Speed = 1500 - (ui_Motors_Speed - 1500);
                        ui_Motors_Speed = 1100;
                        ui_Left_Motor_Speed = constrain(ui_Motors_Speed + ui_Left_Motor_Offset, 1350, 1100);
                        ui_Right_Motor_Speed = constrain(ui_Motors_Speed + ui_Right_Motor_Offset, 1350, 1100);
                    }
                    //END: range met
                    else {
                        ui_Left_Motor_Speed  = ci_Left_Motor_Stop;
                        ui_Right_Motor_Speed = ci_Right_Motor_Stop;
                        subMode++;                    
                    }
                    //END: end range met
                    break;
                case 5:
                    //Turn around
                    if(turnAround()) { ++subMode; }
                    //END: Turned around
                    break;
                case 6:
                    //Drive forward using PID controller
                    followLine(error);
                    static int errorCnt = 0;
                    if (error == 4) errorCnt++;
                    if (errorCnt > 4) {
                        ui_Left_Motor_Speed = ci_Left_Motor_Stop;
                        ui_Right_Motor_Speed = ci_Right_Motor_Stop;
                        subMode++;
                    }
                    //END: LEDS all lit
                    break;
                case 7: {
                    // release flag, idle robot
                    getRange();
                    if(SetReach(getAvgRange())){   // Wait for arm to settle for even an instant
                      OpenGrip(true);              // Drop flag
                      subMode = 0;                 // Reset subMode to 0
                      ui_Robot_State_Index = 0;    // Reset robot to mode 0
                    }
                    //END: All black
                    break;
                        }
                default:
                    break;
                }






                // set motor speeds
                if(digitalRead(ci_EnableMotorsSwitch) == 0)
                {
                    servo_LeftMotor.writeMicroseconds(ci_Left_Motor_Stop);
                    servo_RightMotor.writeMicroseconds(ci_Right_Motor_Stop);
                }
                else
                {
                    servo_LeftMotor.writeMicroseconds(ui_Left_Motor_Speed);
                    servo_RightMotor.writeMicroseconds(ui_Right_Motor_Speed);
                }
# ifdef DEBUG_MOTORS
                Serial.print("Motors: Pot= ");
                Serial.print(ui_Motors_Speed);
                Serial.print(" , Left = ");
                Serial.print(ui_Left_Motor_Speed);
                Serial.print(" . Right = ");
                Serial.println(ui_Right_Motor_Speed);
# endif
                ui_Mode_Indicator_Index = 1;
            }
            break;
        }
    case 2: //Calibrate line tracker light levels after 3 seconds
        {
            if(bt_3_S_Time_Up)
            {
                if(!bt_Cal_Initialized)
                {
                    bt_Cal_Initialized = true;
                    ui_Left_Line_Tracker_Light = 0;
                    ui_Middle_Line_Tracker_Light = 0;
                    ui_Right_Line_Tracker_Light = 0;
                    ul_Calibration_Time = millis();
                    ui_Cal_Count = 0;
                }
                else if((millis() - ul_Calibration_Time) > ci_Line_Tracker_Calibration_Interval)
                {
                    ul_Calibration_Time = millis();
                    readLineTrackers();
                    ui_Left_Line_Tracker_Light += ui_Left_Line_Tracker_Data;
                    ui_Middle_Line_Tracker_Light += ui_Middle_Line_Tracker_Data;
                    ui_Right_Line_Tracker_Light += ui_Right_Line_Tracker_Data;
                    ui_Cal_Count++;
                }
                if(ui_Cal_Count == ci_Line_Tracker_Cal_Measures)
                {
                    ui_Left_Line_Tracker_Light /= ci_Line_Tracker_Cal_Measures;
                    ui_Middle_Line_Tracker_Light /= ci_Line_Tracker_Cal_Measures;
                    ui_Right_Line_Tracker_Light /= ci_Line_Tracker_Cal_Measures;
# ifdef DEBUG_LINE_TRACKER_CALIBRATION
                    Serial.print("Light Levels: Left = ");
                    Serial.print(ui_Left_Line_Tracker_Light,DEC);
                    Serial.print(", Middle = ");
                    Serial.print(ui_Middle_Line_Tracker_Light,DEC);
                    Serial.print(", Right = ");
                    Serial.println(ui_Right_Line_Tracker_Light,DEC);
# endif
                    EEPROM.write(ci_Left_Line_Tracker_Light_Address_L, lowByte(ui_Left_Line_Tracker_Light));
                    EEPROM.write(ci_Left_Line_Tracker_Light_Address_H, highByte(ui_Left_Line_Tracker_Light));
                    EEPROM.write(ci_Middle_Line_Tracker_Light_Address_L, lowByte(ui_Middle_Line_Tracker_Light));
                    EEPROM.write(ci_Middle_Line_Tracker_Light_Address_H, highByte(ui_Middle_Line_Tracker_Light));
                    EEPROM.write(ci_Right_Line_Tracker_Light_Address_L, lowByte(ui_Right_Line_Tracker_Light));
                    EEPROM.write(ci_Right_Line_Tracker_Light_Address_H, highByte(ui_Right_Line_Tracker_Light));
                    ui_Robot_State_Index = 0; // go back to Mode 0
                }
                ui_Mode_Indicator_Index = 2;
            }
            break;
        }
    case 3: // Calibrate line tracker dark levels after 3 seconds
        {
            if(bt_3_S_Time_Up)
            {
                if(!bt_Cal_Initialized)
                {
                    bt_Cal_Initialized = true;
                    ui_Left_Line_Tracker_Dark = 0;
                    ui_Middle_Line_Tracker_Dark = 0;
                    ui_Right_Line_Tracker_Dark = 0;
                    ul_Calibration_Time = millis();
                    ui_Cal_Count = 0;
                }
                else if((millis() - ul_Calibration_Time) > ci_Line_Tracker_Calibration_Interval)
                {
                    ul_Calibration_Time = millis();
                    readLineTrackers();
                    ui_Left_Line_Tracker_Dark += ui_Left_Line_Tracker_Data;
                    ui_Middle_Line_Tracker_Dark += ui_Middle_Line_Tracker_Data;
                    ui_Right_Line_Tracker_Dark += ui_Right_Line_Tracker_Data;
                    ui_Cal_Count++;
                }
                if(ui_Cal_Count == ci_Line_Tracker_Cal_Measures)
                {
                    ui_Left_Line_Tracker_Dark /= ci_Line_Tracker_Cal_Measures;
                    ui_Middle_Line_Tracker_Dark /= ci_Line_Tracker_Cal_Measures;
                    ui_Right_Line_Tracker_Dark /= ci_Line_Tracker_Cal_Measures;
# ifdef DEBUG_LINE_TRACKER_CALIBRATION
                    Serial.print("Dark Levels: Left = ");
                    Serial.print(ui_Left_Line_Tracker_Dark,DEC);
                    Serial.print(", Middle = ");
                    Serial.print(ui_Middle_Line_Tracker_Dark,DEC);
                    Serial.print(", Right = ");
                    Serial.println(ui_Right_Line_Tracker_Dark,DEC);
# endif
                    EEPROM.write(ci_Left_Line_Tracker_Dark_Address_L, lowByte(ui_Left_Line_Tracker_Dark));
                    EEPROM.write(ci_Left_Line_Tracker_Dark_Address_H, highByte(ui_Left_Line_Tracker_Dark));
                    EEPROM.write(ci_Middle_Line_Tracker_Dark_Address_L, lowByte(ui_Middle_Line_Tracker_Dark));
                    EEPROM.write(ci_Middle_Line_Tracker_Dark_Address_H, highByte(ui_Middle_Line_Tracker_Dark));
                    EEPROM.write(ci_Right_Line_Tracker_Dark_Address_L, lowByte(ui_Right_Line_Tracker_Dark));
                    EEPROM.write(ci_Right_Line_Tracker_Dark_Address_H, highByte(ui_Right_Line_Tracker_Dark));
                    ui_Robot_State_Index = 0; // go back to Mode 0
                }
                ui_Mode_Indicator_Index = 3;
            }
            break;
        }
    case 4: //Calibrate motor straightness after 3 seconds
        {
            if(bt_3_S_Time_Up)
            {
                if(!bt_Cal_Initialized)
                {
                    bt_Cal_Initialized = true;
                    CharliePlexing::ul_LeftEncoder_Count = 0;
                    CharliePlexing::ul_RightEncoder_Count = 0;
                    // read pot to set top motor speed
                    ui_Motors_Speed = analogRead(ci_Motor_Speed_Pot);
                    ui_Motors_Speed = map(ui_Motors_Speed, 0, 1023, 1650, 1900); // motor stalls below 1650; too fast above // set motor speeds
                    ui_Left_Motor_Speed = constrain(ui_Motors_Speed, 1650, 1900);
                    ui_Right_Motor_Speed = constrain(ui_Motors_Speed, 1650, 1900);
                    ul_Calibration_Time = millis();
                    servo_LeftMotor.writeMicroseconds(ui_Left_Motor_Speed);
                    servo_RightMotor.writeMicroseconds(ui_Right_Motor_Speed);
                }
                else if((millis() - ul_Calibration_Time) > ci_Motor_Calibration_Time)
                {
                    servo_LeftMotor.writeMicroseconds(ci_Left_Motor_Stop);
                    servo_RightMotor.writeMicroseconds(ci_Right_Motor_Stop);
                    ul_encoder_Pos[0] = CharliePlexing::ul_LeftEncoder_Count;
                    ul_encoder_Pos[1] = CharliePlexing::ul_RightEncoder_Count;
# ifdef DEBUG_ENCODERS
                    for(int i=0; i < ci_Num_Encoders; i++)
                    {
                        Serial.print("Encoder ");
                        Serial.print(i);
                        Serial.print(": ");
                        Serial.println(ul_encoder_Pos[i], DEC);
                    }
# endif
                    if(ul_encoder_Pos[1] > ul_encoder_Pos[0])
                    {
                        ui_Right_Motor_Offset = 0;
                        ui_Left_Motor_Offset = (ul_encoder_Pos[1] - ul_encoder_Pos[0]); // May have to update this if different
                    } else
                    {
                        ui_Right_Motor_Offset = (ul_encoder_Pos[0] - ul_encoder_Pos[1]); // May have to update this if different ui_Left_Motor_Offset = 0;
                    }
# ifdef DEBUG_MOTOR_CALIBRATION
                    Serial.print("Motor Offsets: Right = ");
                    Serial.print(ui_Right_Motor_Offset);
                    Serial.print(", Left = ");
                    Serial.println(ui_Left_Motor_Offset);
# endif
                    EEPROM.write(ci_Right_Motor_Offset_Address_L, lowByte(ui_Right_Motor_Offset));
                    EEPROM.write(ci_Right_Motor_Offset_Address_H, highByte(ui_Right_Motor_Offset));
                    EEPROM.write(ci_Left_Motor_Offset_Address_L, lowByte(ui_Left_Motor_Offset));
                    EEPROM.write(ci_Left_Motor_Offset_Address_H, highByte(ui_Left_Motor_Offset));
                    ui_Robot_State_Index = 0; // go back to Mode 0
                }
                ui_Mode_Indicator_Index = 4;
            }
            break;
        }
    case 5: {
        servo_LeftMotor.writeMicroseconds(1100);
        servo_RightMotor.writeMicroseconds(1200);
        break;
            }
    default:
        {
            ui_Robot_State_Index = 0;
            break;
        }
    }
    if((millis() - ul_Display_Time) > ci_Display_Time)
    {
        ul_Display_Time = millis();
# ifdef DEBUG_MODE_DISPLAY
        Serial.print("Mode: ");
        Serial.println(ui_Mode_Indicator[ui_Mode_Indicator_Index], DEC);
# endif
        bt_Heartbeat = !bt_Heartbeat;
        CharliePlexing::Write(ci_Heartbeat_LED, bt_Heartbeat);
        Indicator();
    }
}

void Indicator()
{
    //display routine, if true turn on led
    CharliePlexing::Write(ci_Indicator_LED,!(ui_Mode_Indicator[ui_Mode_Indicator_Index] & (iArray[iArrayIndex])));
    iArrayIndex++;
    iArrayIndex = iArrayIndex & 15;
}
// read values from line trackers and update status of line tracker LEDs
void readLineTrackers()
{
    ui_Left_Line_Tracker_Data = analogRead(ci_Left_Line_Tracker);
    ui_Middle_Line_Tracker_Data = analogRead(ci_Middle_Line_Tracker);
    ui_Right_Line_Tracker_Data = analogRead(ci_Right_Line_Tracker);
    if(ui_Left_Line_Tracker_Data > (ui_Left_Line_Tracker_Dark - ui_Line_Tracker_Tolerance))
    {
        sensorState[LEFT] = HIGH;
        CharliePlexing::Write(ci_Left_Line_Tracker_LED, HIGH);
    }
    else
    {
        sensorState[LEFT] = LOW;
        CharliePlexing::Write(ci_Left_Line_Tracker_LED, LOW);
    }
    if(ui_Middle_Line_Tracker_Data > (ui_Middle_Line_Tracker_Dark - ui_Line_Tracker_Tolerance))
    {
        sensorState[MIDDLE] = HIGH;
        CharliePlexing::Write(ci_Middle_Line_Tracker_LED, HIGH);
    }
    else
    {
        sensorState[MIDDLE] = LOW;
        CharliePlexing::Write(ci_Middle_Line_Tracker_LED, LOW);
    }
    if(ui_Right_Line_Tracker_Data > (ui_Right_Line_Tracker_Dark - ui_Line_Tracker_Tolerance))
    {
        sensorState[RIGHT] = HIGH;
        CharliePlexing::Write(ci_Right_Line_Tracker_LED, HIGH);
    }
    else
    {
        sensorState[RIGHT] = LOW;
        CharliePlexing::Write(ci_Right_Line_Tracker_LED, LOW);
    }
# ifdef DEBUG_LINE_TRACKERS
    Serial.print("Trackers: Left = ");
    Serial.print(ui_Left_Line_Tracker_Data, DEC);
    Serial.print(", Middle = ");
    Serial.print(ui_Middle_Line_Tracker_Data, DEC);
    Serial.print(", Right = ");
    Serial.println(ui_Right_Line_Tracker_Data, DEC);
# endif
}
// measure distance to target using ultrasonic sensor
void Ping()
{
    //Ping Ultrasonic
    //Send the Ultrasonic Range Finder a 10 microsecond pulse per tech spec
    digitalWrite(ci_Ultrasonic_Ping, HIGH);
    delayMicroseconds(10); //The 10 microsecond pause where the pulse in "high"
    digitalWrite(ci_Ultrasonic_Ping, LOW);
    //use command pulseIn to listen to Ultrasonic_Data pin to record the
    //time that it takes from when the Pin goes HIGH until it goes LOW
    ul_Echo_Time = pulseIn(ci_Ultrasonic_Data, HIGH, 10000);
    // Print Sensor Readings
# ifdef DEBUG_ULTRASONIC
    Serial.print("Time (microseconds): ");
    Serial.print(ul_Echo_Time, DEC);
    Serial.print(", Inches: ");
    Serial.print(ul_Echo_Time/148); //divide time by 148 to get distance in inches
    Serial.print(", cm: ");
    Serial.println(ul_Echo_Time/58); //divide time by 58 to get distance in cm
# endif
}

void serialEvent() {
    while (Serial.available()) {
        char ch = Serial.read();
        switch (ch) {
        case 'g':
            ui_Robot_State_Index = 1;
            Serial.println("Going");
            break;
        case 's':
            ui_Robot_State_Index = 0;
            Serial.println("Stopped");
            break;
        case 'p':
            pid->kP = Serial.readString().toInt();
            Serial.print("kP = ");
            Serial.println(pid->kP);
            // EEPROM_writeAnything<float>(adr_kp, pid->kP);
            break;
        case 'i':
            {
                char n[5];
                String s = Serial.readString();
                s.toCharArray(n,5);
                pid->kI = atof(n);
                Serial.print("kI = ");
                Serial.println(pid->kI);
                //    EEPROM_writeAnything<float>(adr_ki, pid->kI);
            }
            break;
        case 'd':
            {
                char n[5];
                String s = Serial.readString();
                s.toCharArray(n,5);
                pid->kD = atof(n);
                Serial.print("kD = ");
                Serial.println(pid->kD);
                //    EEPROM_writeAnything<float>(adr_kd, pid->kD);
            }
            break;
        case 'r':
            Serial.print("kP = ");
            Serial.println(pid->kP);
            Serial.print("kI = ");
            Serial.println(pid->kI);
            Serial.print("kD = ");
            Serial.println(pid->kD);
            break;
        case 'm':
            Serial.print("RAM = ");
            Serial.println(freeRam());
            break;
        case 'b':
            Serial.println("Backing up");
            ui_Robot_State_Index = 5;
        default:
            break;
        }
    }
}

int freeRam () {
    extern int __heap_start, *__brkval; 
    int v; 
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

bool turnAround() {
    static int encoderCountLeft  = ul_encoder_Pos[0],
               encoderCountRight = ul_encoder_Pos[1];
    static const int halfTurnTicks = 282 + 30; // +30 for slippage compensation
    if (abs(ul_encoder_Pos[0] - encoderCountLeft) < halfTurnTicks) {
        ui_Left_Motor_Speed  = 1000;
        ui_Right_Motor_Speed = 2000;
        return false;
    }
    return true;
}

//Arguement varies from 1650 to 1900
//This function ensures that the robot drives in a straight line
void constrainMotorSpeeds(unsigned int motorSpeed){
  ui_Left_Motor_Speed = constrain(motorSpeed - ui_Left_Motor_Offset, 1620, 1900);
  ui_Right_Motor_Speed = constrain(motorSpeed - ui_Right_Motor_Offset, 1620, 1900);
  Serial.println(ui_Left_Motor_Offset);
   Serial.println(ui_Right_Motor_Offset);
}

//True opens the grip
void OpenGrip(boolean openGrip){
    if(openGrip)
        servo_GripMotor.write(ci_Grip_Motor_Open);
    else
        servo_GripMotor.write(ci_Grip_Motor_Closed);
}

//Returns distance in mm
uint16_t getRange(){
    Ping();

    #ifdef DEBUG_RANGE_FINDING
    Serial.print("Current range: ");
    Serial.print(ul_Echo_Time*10/58);
    Serial.print(" mm");
    #endif
    return (uint16_t)(ul_Echo_Time*10/58);
}

uint16_t getAvgRange(){
    //Ping();  //Can't ping too often (also prevents wasting time)

    const byte entries(5);
    static unsigned long hist[entries] = {0};
    static int index(0);

    hist[index] = ul_Echo_Time;
    index++;
    if (index==entries) index = 0;

    unsigned long dist(0);
    for (int j=0; j<entries; j++) {
        if (hist[j]==0) hist[j]=ul_Echo_Time;
        dist += hist[j];
    }

    #ifdef DEBUG_RANGE_FINDING
    Serial.print("Average range over last ");
    Serial.print(entries);
    Serial.print(" readings: ");
    Serial.print(dist*10/entries/58);
    Serial.println(" mm");
    #endif
    return (uint16_t)(dist/58);
}

bool SetReach(int reach){

    bool stable(false);

    const int ci_MaxCM = 190;
    const int ci_MinCM = 110;
    const int ci_Max_Length = 1010;
    const int ci_Min_Length = 150;
    const int ci_Arm_Extend_Speed = 1200;
    const int ci_Arm_Retract_Speed = 1800;
    int i_Arm_Length = analogRead(ci_Arm_Length_Pot);

    //Convert cm to distance
    reach = constrain(reach, ci_MinCM, ci_MaxCM);
    reach = map(reach, ci_MinCM, ci_MaxCM, ci_Min_Length, ci_Max_Length);

    if(i_Arm_Length>(reach+150)){
        servo_ArmMotor.writeMicroseconds(ci_Arm_Retract_Speed);  //Pull back from too far forward
    }
    else if(i_Arm_Length>(reach+100)){
        servo_ArmMotor.writeMicroseconds(1600);  //Pull back slowly from slightly too far forward
    }
    else if(i_Arm_Length<(reach-150)){
        servo_ArmMotor.writeMicroseconds(ci_Arm_Extend_Speed);   //Push forward from too far back
    }
    else if(i_Arm_Length<(reach-100)){
        servo_ArmMotor.writeMicroseconds(1400);  //Push forward slowly from slightly too far back
    }
    else{
        servo_ArmMotor.writeMicroseconds(200);                   //Acceptably close, stop motor
        stable = true;
    }

    #ifdef DEBUG_ARM_CONTROL
    Serial.print("Current reach: ");
    Serial.println(i_Arm_Length);
    Serial.print("Desired reach: ");
    Serial.println(reach);
    Serial.print("Arm movement stable: ");
    Serial.println(stable);
    #endif

    return stable;
}

void followLine(int8_t error) {
    static float correction  = 0;
    static unsigned int baseMotorSpeed = MAX_SPEED_FWD; //1600 + (1900 - 1600)/2;
    if (error == 4) {
        ui_Left_Motor_Speed  = ci_Left_Motor_Stop;
        ui_Right_Motor_Speed = ci_Right_Motor_Stop;
    } else {
        correction           = pid->update(error);
        ui_Left_Motor_Speed  = constrain(baseMotorSpeed - correction, MAX_SPEED_REV, MAX_SPEED_FWD);
        ui_Right_Motor_Speed = constrain(baseMotorSpeed + correction, MAX_SPEED_REV, MAX_SPEED_FWD);
#ifdef DEBUG_PID_CONTROLLER
        Serial.print(pid->proportional);
        Serial.print('\t');
        Serial.print(pid->integral);
        Serial.print('\t');
        Serial.print(pid->derivative);
        Serial.print('\t');
        Serial.print(correction);
        Serial.print('\t');
        Serial.print(ui_Left_Motor_Speed);
        Serial.print('\t');
        Serial.println(ui_Right_Motor_Speed);
#endif
    }
}