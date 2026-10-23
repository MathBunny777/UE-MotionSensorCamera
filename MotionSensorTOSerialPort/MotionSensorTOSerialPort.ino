#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>  

Adafruit_BNO055 bno = Adafruit_BNO055(55);

void setup(void) 
{
  Serial.begin(9600);
  Serial.println("Orientation Sensor Test"); Serial.println("");
  
  // Initialise the sensor
  while (!bno.begin())
  {
    // There was a problem detecting the BNO055 ... check your connections
    Serial.println("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    delay(1000);
  }
  delay(1000);
  
  int temp = bno.getTemp();
  Serial.print("Temp: ");
  Serial.println(temp);
  delay(2000);

  uint8_t system, gyro, accel, mag;
  system = gyro = accel = mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag); /* The data should be ignored until the system calibration is &gt; 0 */
  Serial.print("\t");
  if (!system)
  {
    Serial.print("! ");
  }

  Serial.print("Sys:");
  Serial.print(system, DEC);
  Serial.print(" G:");
  Serial.print(gyro, DEC);
  Serial.print(" A:");
  Serial.print(accel, DEC);
  Serial.print(" M:");
  Serial.println(mag, DEC);

  Adafruit_BNO055::adafruit_bno055_rev_info_t rev;
  bno.getRevInfo(&rev);

  bno.setExtCrystalUse(true);
  delay(5000);
}

void loop(void) 
{
  // Get a new sensor event
  sensors_event_t event; 
  bno.getEvent(&event);
  
  char tmp[128] = {};
  sprintf(tmp, "R:%.0f,%.0f,%.0f",  event.orientation.x,  event.orientation.y,  event.orientation.z);
  Serial.println(tmp);

  delay(32); // delay in ms, roughly 30fps
}