//Board = Arduino Duemilanove w/ ATmega328
#define ARDUINO 103
#define __AVR_ATmega328P__
#define F_CPU 16000000L
#define __AVR__
#define __cplusplus
#define __attribute__(x)
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__
#define __builtin_va_list
#define __builtin_va_start
#define __builtin_va_end
#define __DOXYGEN__
#define prog_void
#define PGM_VOID_P int
#define NOINLINE __attribute__((noinline))

typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {}

int8_t calcError(uint8_t sensorStates);
//already defined in arduno.h
//already defined in arduno.h
void Indicator();
void readLineTrackers();
void Ping();
void serialEvent();
int freeRam ();
bool turnAround();
void constrainMotorSpeeds(unsigned int motorSpeed);
void OpenGrip(boolean openGrip);
uint16_t getRange();
uint16_t getAvgRange();
bool SetReach(int reach);
void followLine(int8_t error);

#include "D:\Program Files\arduino-1.0.3\hardware\arduino\variants\standard\pins_arduino.h" 
#include "D:\Program Files\arduino-1.0.3\hardware\arduino\cores\arduino\arduino.h"
#include "D:\My Documents\Arduino\MSE_2202_Lab_04\MSE_2202_Lab_04.ino"
