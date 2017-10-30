# PM and CO Data Logger
This sketch gets readings from an DFRobot MQ7 Carbon Monoxide Sensor 
and Plantower PM3003 Particulate Matter Sensor. An Arduino Uno R3 is used
with an Adafruit Ultimate GPS Logger Shield to get timestamps and to have SD card 
logging functionality. An Adafruit HTU21DF sensor was used to get relative humidity
and temperature measurements. Readings are written in a CSV file with the following
format per line:

`CO, TEMP, HUM, PM1_CF1, PM2.5_CF1, PM10_CF1, PM1_ATM, PM2.5_ATM, PM10_ATM, TIMESTAMP, LAT, LONG, ALT`

where CF1 and ATM refers to PM readings at CF=1 and under atmospheric
conditions, respectively (refer to PM3003 manual).
