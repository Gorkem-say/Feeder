# Feeder


FEEDER project includes PCB and schematics design. The circuit consists of ATMEGA329P AU mcu, HX711 amplifier for load cell and UA2003 stepper motor driver. There are several programming codes for different operations.
One of the operation is to just drive stepper driver. The arduino IDE file is STEPPERDRIVERFORFEEDER. 
Other operation is to set 2 schedules to feed animals at specific weight of the food. the load cell can be calibrated if button is pressed. This IDE file is fsmwithtime.





Example code is shown below. The code can calibrate load cell when button cell is pressed. Also 2 scheduled feeding process is adjusted. Moreover RTC is connected. Target weight is 35 gram


#include "RTClib.h"

//#include <TimeLib.h>
//#include <DS1307RTC.h>
#include <Wire.h>



#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif
#include <AccelStepper.h>

const int HX711_dout = 3; //mcu > HX711 dout pin
const int HX711_sck = 4; //mcu > HX711 sck pin
volatile float i;
const int motorPin1 = 8; // IN1
const int motorPin2 = 7; // IN2
const int motorPin3 = 6; // IN3
const int motorPin4 = 5; // IN4
const int buttonPin = A0; // Button pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;

AccelStepper stepper(AccelStepper::FULL4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);

const int forwardDuration = 3000; // 2 seconds forward
const int reverseDuration = 500; // 1 second reverse

float targetWeight = 35.0;
RTC_DS1307 rtc;

void setup() {
            // put your setup code here, to run once:
           Serial.begin(57600); 
           delay(10);
           Serial.println();
           Serial.println("Starting...");
            LoadCell.begin();
      float newCalibrationValue = 0.0;
            //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
            unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
            boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
            LoadCell.start(stabilizingtime, _tare);

            #if defined(ESP8266)|| defined(ESP32)
    EEPROM.begin(512); // Use the correct EEPROM size for your Arduino board
    #endif
     EEPROM.get(calVal_eepromAdress, newCalibrationValue);
    LoadCell.setCalFactor(newCalibrationValue);
    

          
            stepper.setMaxSpeed(500); // Adjust this to your motor's maximum speed
            stepper.setAcceleration(100); // Adjust this to your desired acceleration
          
            // Set the initial direction to forward
            stepper.setSpeed(500); // Adjust the speed as needed for forward direction
                
            pinMode(buttonPin, INPUT_PULLUP); // Set the button pin as input with internal pull-up resistor
 
  
            Wire.begin();  // Initialize I2C communication
            rtc.begin();

 if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
     rtc.adjust(DateTime(2023, 10, 9, 8, 0, 0));
  }
            
             }

    #define  BOOT 1
    #define  CALIBRATE_BEGIN 2
    #define  CALIBRATE_TARE 3
    #define  CALIBRATE_MEASURE 4
    #define  CALIBRATE_PUT_MASS 5
    #define  CALIBRATE_PUT_MASS_HIGH 6
    #define  CALIBRATE_SAVE 7
    float newCalibrationValue = 0.0;

   
 //   Wire.begin();

void fsm(){
  static int state = 1;
  EEPROM.get(calVal_eepromAdress, newCalibrationValue); 
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0;
   if (LoadCell.update()) newDataReady = true;
  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i);
      newDataReady = 0;
      t = millis();
    }
  }  

    
  switch(state) {   
    case BOOT:                                         // ilk butona basışta calibrate başlar
    if (digitalRead(buttonPin) == LOW){
    Serial.println("case boot tı calibrate begin");
      state = CALIBRATE_BEGIN;}
    break;
  case CALIBRATE_BEGIN:                              // butona basma bırakılınca tare alır
    if (digitalRead(buttonPin) == HIGH)
      state = CALIBRATE_TARE;
    break;
  case CALIBRATE_TARE:
      LoadCell.tareNoDelay();
    state = CALIBRATE_PUT_MASS;
    break;

  case CALIBRATE_PUT_MASS:
    if (digitalRead(buttonPin) == LOW){                      // ikinci butona basışta calibrate başlar
    Serial.println("case put mass to high");
      state = CALIBRATE_PUT_MASS_HIGH;}
    break;
  case CALIBRATE_PUT_MASS_HIGH:                             // butona basma bırakılınca bilinen ağırlığı kalibre eder ve eeproma kaydeder
    if (digitalRead(buttonPin) == HIGH)
      state = CALIBRATE_SAVE;
    break;
  case CALIBRATE_SAVE:
    float known_mass= 46.0;
    LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
    newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value
    LoadCell.setCalFactor(newCalibrationValue);
    #if defined(ESP8266)|| defined(ESP32)
    EEPROM.begin(512);
    #endif
    EEPROM.put(calVal_eepromAdress, newCalibrationValue);
    #if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();      
    #endif
    EEPROM.get(calVal_eepromAdress, newCalibrationValue);
      Serial.print("Value ");
      Serial.print(newCalibrationValue);
      Serial.print(" saved to EEPROM address: ");
      Serial.println(calVal_eepromAdress);

  
    state = BOOT;
    break;
 }
}

#define  MOTOR_IDLE 1
#define  MOTOR_FORWARD 2
#define  MOTOR_REVERSE 3

void motor(){
  static int state = 1;
  static auto t_begin = millis();
  
  switch(state) {
    case MOTOR_IDLE:
    stepper.setSpeed(0);
    //if (i < targetWeight){
    //   t_begin = millis();
    //   state = MOTOR_FORWARD;
    //}
    t_begin = millis();
    state = MOTOR_FORWARD;
    break;

  case MOTOR_FORWARD:
    stepper.setSpeed(-1000);
    if (millis() - t_begin >= forwardDuration){
        t_begin = millis();
       state = MOTOR_REVERSE;
    }
    
    break;

  case MOTOR_REVERSE:
    stepper.setSpeed(1000);
    if (millis() - t_begin >= reverseDuration){
      t_begin = millis();
       state = MOTOR_FORWARD;
    }
    break;
}
 stepper.runSpeed();
}


 #define DATE_IDLE 1
 #define FEED 2
  #define FEED_WAIT 3

 void date(){
   static int state = 1;
   DateTime now = rtc.now();
   static auto t_begin = millis();
   
    
    switch(state) {
    case DATE_IDLE:
     if ((now.hour() == 8 && now.minute() == 1 && i < targetWeight) || (now.hour() == 8 && now.minute() == 3 && i < targetWeight)) {            // tarih ve ağırlık şartları sağlandığında feed komutunu başlatır
    state = FEED;
     }
    break;
    
    case FEED:
    motor();                                                                       // motor() başlar hedef ağırlığa ulaşıncaya kadar devam eder. hedef ağırlığa ulaşınca FEED_WAIT başlar
     if (i > targetWeight) {     // Check if it's time to feed
     t_begin = millis();
    state = FEED_WAIT;
     
    }
     break;

     case FEED_WAIT:                                                              // 60 saniye bekletilir sonra DATE_IDLE a geri döner.
     stepper.setSpeed(0);
      if(millis() - t_begin >= 60000){
         t_begin = millis();
         state = DATE_IDLE;
    
    }
     break;
   
   
  }
 
 }
void loop() {
 
   DateTime now = rtc.now();
    fsm();
    date();
    
Serial.println(now.minute(), DEC);
  
}
