#include <arduino.h>
#include "task.h"

//Configuration LED
//const int LEDBoard = 2;  //DevModule
//const int LEDBoard = 13;   //Adafruit Huzzah32

enum LED {        // RG LED 33,25 (not used 23), RGB 25,33,36
  Red = 33, 
  Green = 25, 
  Blue = 26,
  LEDBoard = 13 //Adafruit Huzzah32
  };

void LEDblink(int PIN = LED()){
 taskBegin();
   while(1)   // blockiert dank der TaskPause nicht 
   {
      digitalWrite(PIN,HIGH);  // LED ein
      taskPause(250);   // gibt Rechenzeit ab         
      digitalWrite(PIN,LOW);   // LED aus
      taskPause(1000);   // gibt Rechenzeit ab         
   }
   taskEnd();   
}

void LEDflash(int PIN = LED()){
   taskBegin();
   while(1)   // blockiert dank der TaskPause nicht 
   {
      digitalWrite(PIN,HIGH);  // LED ein
      delay (5);
      //taskPause(2);   // gibt Rechenzeit ab    
      digitalWrite(PIN,LOW);   // LED aus
      taskPause(3000);   // gibt Rechenzeit ab    
   }
   taskEnd();   
}

void LEDInit() {
  pinMode(LED(Red),   OUTPUT);
  pinMode(LED(Blue),  OUTPUT);
  pinMode(LED(Green), OUTPUT);
  digitalWrite(LED(Red), 1);
  delay(250);
  digitalWrite(LED(Red), 0);
  digitalWrite(LED(Blue), 1);
  delay(250);
  digitalWrite(LED(Blue), 0);
  digitalWrite(LED(Green), 1);
  delay(250);
  digitalWrite(LED(Green), 0);
}

void LEDoff() {
  digitalWrite(LED(Blue), 0);
  digitalWrite(LED(Green), 0);
  digitalWrite(LED(Red), 0);
}

