/*PM and CO Data Logger*/
/*This sketch gets readings from an Adafruit MQ7 Carbon Monoxide Sensor 
and Plantower PM3003 Particulate Matter Sensor. An Arduino Uno R3 is used
with an Adafruit Ultimate GPS Logger Shield to get timestamps and to have SD card 
logging functionality. An Adafruit HTU21DF sensor was used to get relative humidity
and temperature measurements. Readings are written in a CSV file with the following
format per line:

CO, TEMP, HUM, PM1_CF1, PM2.5_CF1, PM10_CF1, PM1_ATM, PM2.5_ATM, PM10_ATM, TIMESTAMP, LAT, LONG, ALT

where CF1 and ATM refers to PM readings at CF=1 and under atmospheric
conditions, respectively (refer to PM3003 manual).
*/

#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GPS.h>
#include "Adafruit_HTU21DF.h"

/*Global variables*/
SoftwareSerial GPS_Serial(8, 7); // RX, TX - serial port connected to GPS Module
SoftwareSerial PM_Serial(3, 2); // RX, TX - serial port connected to PM Sensor
Adafruit_GPS GPS(&GPS_Serial);
File logFile;
const char* FILENAME = "LOG.CSV";
byte STARTBYTE = 0x42; //start byte of PM3003 sensor
int PMPACKETLENGTH = 24; //length of data packet from PM3003
int CSPIN = 10; //SD CS Pin
int BAUDRATE = 9600;
char DELIMITER = ',';
int TIME_INTERVAL = 5000; //interval between measurements in milliseconds
bool DEBUG = false; //switch to true when USB serial port output is needed. false otherwise.

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

void setup() {

  //initialize serial port if in debug mode
  if (DEBUG)
    // Open serial communications and wait for port to open:
    Serial.begin(BAUDRATE);

  // set the data rate for the SoftwareSerial port
  GPS.begin(BAUDRATE);

  // turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate

  delay(1000);

  PM_Serial.begin(BAUDRATE);

  //flush serial ports
  if (DEBUG)
    Serial.flush();
  PM_Serial.flush();

  //set CS pin as output
  pinMode(CSPIN, OUTPUT);

  //check SD
  if (!SD.begin(CSPIN)) {
    if (DEBUG)
      Serial.println("initialization failed!");
    return;
  }

  if (!htu.begin()) {
    if (DEBUG)
      Serial.println("Couldn't find sensor!");
  }

}


//helper function to format time units (month, day, hour, minute, second) 
//to two places by appending '0' if it is less than 10
String formatTimeUnit(int timeUnit){
  String formattedTimeUnit = String(timeUnit, DEC);
  
  if (timeUnit < 10){
    formattedTimeUnit.remove(0);
    formattedTimeUnit.concat('0');
    formattedTimeUnit.concat(String(timeUnit, DEC));
  }

  return formattedTimeUnit;
}


//gets timestamp from GPS (format: YYMMDDHHmmSS)
String getTime(){

  String timestamp = "";

  timestamp.concat(formatTimeUnit(GPS.year));
  timestamp.concat(formatTimeUnit(GPS.month));
  timestamp.concat(formatTimeUnit(GPS.day));
  timestamp.concat(formatTimeUnit(GPS.hour));
  timestamp.concat(formatTimeUnit(GPS.minute));
  timestamp.concat(formatTimeUnit(GPS.seconds));

  if (DEBUG)
    Serial.println("timestamp: " + timestamp);
  
  return timestamp;
}

//gets coordinates from GPS (format: lat,long)
String getCoordinates(){

  String coordinates = "";

  //check if GPS locked. if not, just put "nf"(no fix)
  if (GPS.fix){
    coordinates.concat(GPS.latitude);
    coordinates.concat(DELIMITER);
    coordinates.concat(GPS.longitude);
  }else {
    coordinates.concat("nf,nf");
  }

  if (DEBUG)
    Serial.println("coordinates: " + coordinates);
  
  return coordinates;
}

//gets altitude from GPS (format: lat,long)
String getAltitude(){

  String altitude = "";

  //check if GPS locked. if not, just put "nf"(no fix)
  if (GPS.fix){
    altitude.concat(GPS.altitude / 100.0);
  }else{
    altitude.concat("nf");
  }

  if (DEBUG)
    Serial.println("altitude: " + altitude);
  
  return altitude;
}

float getTemperature(){
  float temperature = htu.readTemperature();

  if (DEBUG){
    Serial.print("temperature: ");
    Serial.println(temperature);
  }

  return temperature;
}

float getHumidity(){
  float humidity = htu.readHumidity();

  if (DEBUG){
    Serial.print("humidity: ");
    Serial.println(humidity);
  }

  return humidity;
}


//get analog voltage output from CO sensor (Adafruit MQ7).
int getCO(){
  
  int CO_Output;

  CO_Output = analogRead(A0);

  if (DEBUG)
    Serial.println("CO output: " + String(CO_Output));
  
  return CO_Output;
}


//get data bytes from data packet of PM sensor
String getPM(char delimiter) {
  
  int count = 0;
  String PM = "";
  
  while (count < PMPACKETLENGTH - 1){
    if(PM_Serial.available()){
      //get bytes 3 to 14 (data bytes from PM3003, refer to manual)
      if(count >= 3 && count <= 14){
        PM.concat(String(PM_Serial.read(), HEX));

        //separate every 2 bytes
        if(count % 2 == 0)
          PM.concat(delimiter);
        
        count++;
      }else{
        PM_Serial.read();
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
  PM_Serial.flush();

  //open file for writing
  if (!logFile)
    logFile = SD.open(FILENAME, FILE_WRITE);

  //get CO reading
  logFile.print(getCO());
  logFile.print(DELIMITER);

  //get temperature
  logFile.print(getTemperature());
  logFile.print(DELIMITER);

  //get humidity
  logFile.print(getHumidity());
  logFile.print(DELIMITER);

  //activate PM serial port
  PM_Serial.listen();

  //wait for start byte from PM sensor then get data packets
  while(1){
    if (PM_Serial.available()){
      if (PM_Serial.read() == STARTBYTE){
        logFile.print(getPM(DELIMITER));
        break;
      }
    }
  }

  //activate GPS serial port
  GPS_Serial.listen();

  //wait for NMEA sentence
  while(1){
    GPS.read();
    
    if (GPS.newNMEAreceived()){
      if (GPS.parse(GPS.lastNMEA()))
        break; 
    }
  }

  //get time from GPS
  logFile.print(getTime());
  logFile.print(DELIMITER);

  //get coordinates from GPS
  logFile.print(getCoordinates());
  logFile.print(DELIMITER);

  //get altitude from GPS
  logFile.println(getAltitude());

  //close file
  logFile.close();

  //pause before getting next measurement
  delay(TIME_INTERVAL);

}
