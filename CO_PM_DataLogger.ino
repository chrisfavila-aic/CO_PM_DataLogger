/*PM and CO Data Logger*/
/*This sketch gets readings from an Adafruit MQ7 Carbon Monoxide Sensor 
and Plantower PM3003 Particulate Matter Sensor. An Arduino Uno R3 is used
with an Adafruit Data Logging Shield to get timestamps and to have SD card 
logging functionality. Readings are written in a CSV file with the following
format per line:

CO, PM1_CF1, PM2.5_CF1, PM10_CF1, PM1_ATM, PM2.5_ATM, PM10_ATM, TIMESTAMP

where CF1 and ATM refers to PM readings at CF=1 and under atmospheric
conditions, respectively (refer to PM3003 manual).
*/

#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

/*Global variables*/
SoftwareSerial mySerial(7, 8); // RX, TX - serial port connected to PM sensor
RTC_PCF8523 rtc;
File myFile;
const char* FILENAME = "LOG.CSV";
int sensorValue;
byte STARTBYTE = 0x42; //start byte of PM3003 sensor
int PMPACKETLENGTH = 24; //length of data packet from PM3003
int CSPIN = 10; //SD CS Pin
int BAUDRATE = 9600;
char DELIMITER = ',';
int TIME_INTERVAL = 5000; //interval between measurements in milliseconds
bool DEBUG = false; //switch to true when USB serial port output is needed. false otherwise.


void setup() {

  //initialize serial port if in debug mode
  if (DEBUG)
    // Open serial communications and wait for port to open:
    Serial.begin(BAUDRATE);

  //check RTC
  if (! rtc.begin()) {
    if (DEBUG)
      Serial.println("Couldn't find RTC");
    while (1);
  }

  //check if RTC is initialized
  if (! rtc.initialized()) {
    if (DEBUG)
      Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // set the data rate for the SoftwareSerial port
  mySerial.begin(BAUDRATE);
  while (!mySerial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //flush serial ports
  if (DEBUG)
    Serial.flush();
  mySerial.flush();

  //check SD
  if (!SD.begin(CSPIN)) {
    if (DEBUG)
      Serial.println("initialization failed!");
    return;
  }

}


//helper function to format time units (month, day, hour, minute, second) 
//to two places by appending '0' if it is less than 10
String formatTimeUnit(int timeUnit){
  String formattedTimeUnit = String(timeUnit, DEC);
  
  if (timeUnit < 10){
    formattedTimeUnit = "0" + formattedTimeUnit;
  }

  return formattedTimeUnit;
}


//gets timestamp from RTC (format: YYYYMMDDHHmmSS). RTC is part of Adafruit Data Logging Shield
String getTime(){
  DateTime now = rtc.now();

  String timestamp = "";
  
  timestamp.concat(String(now.year(), DEC));
  timestamp.concat(formatTimeUnit(now.month()));
  timestamp.concat(formatTimeUnit(now.day()));
  timestamp.concat(formatTimeUnit(now.hour()));
  timestamp.concat(formatTimeUnit(now.minute()));
  timestamp.concat(formatTimeUnit(now.second()));

  if (DEBUG)
    Serial.println("timestamp: " + timestamp);
  
  return timestamp;
}


//get analog voltage output from CO sensor (Adafruit MQ7).
String getCO(){
  
  sensorValue = analogRead(A0);
  
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);

  if (DEBUG)
    Serial.println("CO sensor voltage: " + String(voltage));
  
  return String(voltage);
}


//get data bytes from data packet of PM sensor
String getPM(char delimiter) {
  
  int count = 0;
  String PM = "";
  
  while (count < PMPACKETLENGTH - 1){
    if(mySerial.available()){
      //get bytes 3 to 14 (data bytes from PM3003, refer to manual)
      if(count >= 3 && count <= 14){
        PM.concat(String(mySerial.read(), HEX));

        //separate every 2 bytes
        if(count % 2 == 0)
          PM.concat(delimiter);
        
        count++;
      }else{
        mySerial.read();
        count++;
      }
    }
  }

  if (DEBUG)
    Serial.println("PM Data: " + PM);
      
  return PM;
}



void loop(){

  //flush serial ports
  if (DEBUG)
    Serial.flush();
  mySerial.flush();

  //open file for writing
  if (!myFile)
    myFile = SD.open(FILENAME, FILE_WRITE);

  //get CO reading
  myFile.print(getCO() + DELIMITER);

  //wait for start byte from PM sensor then get data packets
  while(1){
    if (mySerial.available()){
      if (mySerial.read() == STARTBYTE){
        myFile.print(getPM(DELIMITER));
        break;
      }
    }
  }

  //get time from RTC
  myFile.println(getTime());

  //close file
  myFile.close();

  //pause before getting next measurement
  delay(TIME_INTERVAL);

}
