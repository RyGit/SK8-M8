#include <SoftwareSerial.h>

#include <BLEAttribute.h>
#include <BLECentral.h>
#include <BLECharacteristic.h>
#include <BLECommon.h>
#include <BLEDescriptor.h>
#include <BLEPeripheral.h>
#include <BLEService.h>
#include <BLETypedCharacteristic.h>
#include <BLETypedCharacteristics.h>
#include <BLEUuid.h>
#include <CurieBLE.h>
#include <CurieIMU.h>
#include <MadgwickAHRS.h>

#define rxPin 2
#define txPin 3

Madgwick filter;
unsigned long microsPerReading, microsPrevious;
float accelScale, gyroScale;
String motionReading;

BLEPeripheral blePeripheral;
BLEService bleService("180D");

// Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

BLECharacteristic motionSensor("2A37", BLERead | BLENotify, 8);

const int ledPin = 13; //pin for LED

SoftwareSerial blueTooth(rxPin, txPin);


void setup() {
  Serial.begin(9600);
  blueTooth.begin(9600);

  // start the IMU and filter
  CurieIMU.begin();
  CurieIMU.setGyroRate(25);
  CurieIMU.setAccelerometerRate(25);
  filter.begin(25);

  // Set the accelerometer range to 2G
  CurieIMU.setAccelerometerRange(2);
  // Set the gyroscope range to 250 degrees/second
  CurieIMU.setGyroRange(250);

  // initialize variables to pace updates to correct rate
  microsPerReading = 1000000 / 25;
  microsPrevious = micros();

  // set LED pin to output mode
  pinMode(ledPin, OUTPUT);

  // set advertised local name and service UUID:
  blePeripheral.setLocalName("SK8 M8");
  blePeripheral.setAdvertisedServiceUuid(bleService.uuid());

  // add service and characteristic:
  blePeripheral.addAttribute(bleService);
  blePeripheral.addAttribute(switchCharacteristic);
  blePeripheral.addAttribute(motionSensor);

  // set the initial value for the characeristic:
  switchCharacteristic.setValue(0);

  // begin advertising BLE service:
  blePeripheral.begin();

  Serial.println("BLE LED Peripheral");

}

void loop() 
{
  // put your main code here, to run repeatedly:
  int aix, aiy, aiz;
  int gix, giy, giz;
  float ax, ay, az;
  float gx, gy, gz;
  float roll, pitch, heading;
  unsigned long microsNow;

  // listen for BLE peripherals to connect:
  BLECentral central = blePeripheral.central();

  // if a central is connected to peripheral:
  if (central) 
  {
    while (central.connected())
    {
      digitalWrite(ledPin, HIGH);
      Serial.println(("Device Connected"));
      // check if it's time to read data and update the filter
      microsNow = micros();
      if (microsNow - microsPrevious >= microsPerReading)
      {
        // read raw data from CurieIMU
        CurieIMU.readMotionSensor(aix, aiy, aiz, gix, giy, giz);

        // convert from raw data to gravity and degrees/second units
        ax = convertRawAcceleration(aix);
        ay = convertRawAcceleration(aiy);
        az = convertRawAcceleration(aiz);
        gx = convertRawGyro(gix);
        gy = convertRawGyro(giy);
        gz = convertRawGyro(giz);

        // update the filter, which computes orientation
        filter.updateIMU(gx, gy, gz, ax, ay, az);

          // print the heading, pitch and roll
          roll = filter.getRoll();
          pitch = filter.getPitch();
          heading = filter.getYaw();
          Serial.print("Orientation: ");
          blueTooth.write(heading);
          Serial.print(heading);
          Serial.print(" ");
          blueTooth.write(pitch);
          Serial.print(pitch);
          Serial.print(" ");
          blueTooth.write(roll);
          Serial.println(roll);


        // increment previous time, so we keep proper pace
        microsPrevious = microsPrevious + microsPerReading;
      }
    }
  }

    // when the central disconnects, print it out:
    digitalWrite(ledPin, LOW);
    Serial.println(("No Device Connected."));
 }

float convertRawAcceleration(int aRaw)
{
  // since we are using 2G range
  // -2g maps to a raw value of -32768
  // +2g maps to a raw value of 32767
 
  float a = (aRaw * 2.0) / 32768.0;
  return a;
}

float convertRawGyro(int gRaw)
{
  // since we are using 250 degrees/seconds range
  // -250 maps to a raw value of -32768
  // +250 maps to a raw value of 32767
 
  float g = (gRaw * 250.0) / 32768.0;
  return g;
}

