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

BLECharCharacteristic rollReading("12B10001-E8F2-537E-4F6C-D104768A1214", BLERead);
BLECharCharacteristic pitchReading("13B10001-E8F2-537E-4F6C-D104768A1214", BLERead);
BLECharCharacteristic headingReading("14B10001-E8F2-537E-4F6C-D104768A1214", BLERead); // Fowards and Backwards

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
  blePeripheral.addAttribute(rollReading);
  blePeripheral.addAttribute(pitchReading);
  blePeripheral.addAttribute(headingReading);

  // set the initial value for the characeristic:
  pitchReading.setValue(7);
  rollReading.setValue(9);
  pitchReading.setValue(5);

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
  int roll, pitch, heading;


  // listen for BLE peripherals to connect:
  BLECentral central = blePeripheral.central();

  // if a central is connected to peripheral:
  if (central) 
  {
    //motionSensor.setValue(6);
    
    while (central.connected())
    {
      digitalWrite(ledPin, HIGH);
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

       roll = filter.getRoll();
       pitch = filter.getPitch();
       heading = filter.getYaw();

       rollReading.setValue(roll);
       pitchReading.setValue(pitch);
       headingReading.setValue(heading);

       digitalWrite(ledPin, LOW);
       
     }
    }
    
    digitalWrite(ledPin, LOW);
    Serial.println(("No Device Connected."));
  }

    // when the central disconnects, print it out:



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

