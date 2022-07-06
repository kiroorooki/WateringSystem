#include <Arduino.h>

//Pump pins
const int APUMP_PIN = 5;
const int BPUMP_PIN = 3;
const int CPUMP_PIN = 4;
const int DPUMP_PIN = 5;

//Sensor pins, to pair with pump pins
#define AMOISTURE_SENSOR_PIN 34
#define BMOISTURE_SENSOR_PIN 35
#define CMOISTURE_SENSOR_PIN 36
#define DMOISTURE_SENSOR_PIN 39

const int MSENSORS_NUMBER = 1;
const int MSENSORS_READ_NUMBER = 100;
const int MSENSOR_MAX_VALUE = 600;
const int MSENSOR_MIN_VALUE = 400;
const float MSENSOR_DRY_PERCENT = .6;
const float MSENSOR_DRY_VALUE = MSENSOR_MIN_VALUE + (MSENSOR_MAX_VALUE - MSENSOR_MIN_VALUE)*.6;
const float MSENSOR_TIME_BETWEEN_READ_MS = 600000; //3600000;1h

const int PUMP_NUMBER = 1;
const float PUMP_BANDWITH_IN_MLSEC = 21.7;
const float PUMP_TIME_INMS_FOR_1ML = 46.08;
const float PUMP_MAX_VOLUME_ONE_GO_IN_ML = 150;
const float PUMP_TIME_BETWEEN_POURS_MS = 60000;

//Volume of water to pour per plant 
const int BASIL_WATER_VOLUME_ML = 250;
const int PEPPER_WATER_VOLUME_ML = 150;
const int PERSIL_WATER_VOLUME_ML = 250;
const int CIBOUL_WATER_VOLUME_ML = 200;
const int LEMON_WATER_VOLUME_ML = 300;
const int MINT_WATER_VOLUME_ML = 400;

enum pumpName {A, B, C, D};
bool pumps[] = {false, false, false, false};
float sensorsValues[] = {0,0,0,0};

void resetBuffer(int *buf)
{
  int size = sizeof(buf) / sizeof(int);
  for (int i = 0; i < size; i++)
  {
    buf[i] = 0;
  }
}

void resetBuffer(float *buf)
{
  int size = sizeof(buf) / sizeof(float);
  for (int i = 0; i < size; i++)
  {
    buf[i] = 0;
  }
}

//A sensor is associated to a pump
float getMoistureSensorValue(pumpName pump)
{
  float sensorRead = 0;
  switch (pump)
  {
  case A:
    sensorRead = analogRead(AMOISTURE_SENSOR_PIN);
    return sensorRead;
    break;
  
  case B:
    sensorRead = analogRead(BMOISTURE_SENSOR_PIN);
    return sensorRead;
    break;

  case C:
    sensorRead = analogRead(CMOISTURE_SENSOR_PIN);
    return sensorRead;
  break;

  case D:
    sensorRead = analogRead(DMOISTURE_SENSOR_PIN);
    return sensorRead;
  break;

  default:
    return -1;
    break;
  }
}

//Read the moisture sensors,  
//NumberOfReads is the number of time each sensors is being read to round the reading for 1 read
//Always 1ms between reads
void readMoistureSensors(float *buf, int numberOfReads)
{
  Serial.print("Read sensors \n");
  for (int i = 0; i < numberOfReads; i++) 
  { 
    for(int j = 0; j < MSENSORS_NUMBER; j++)
    {
      float read = getMoistureSensorValue(pumpName(j));
      buf[j] = buf[j] + read;
    } 
    delay(1);
  }
  for (int i = 0; i < MSENSORS_NUMBER; i++)
  {
    buf[i] = buf[i] / numberOfReads;
  }
}

float getDurationFromMililiter(float mililiters)
{
  return (mililiters / PUMP_BANDWITH_IN_MLSEC) * 1000;
}

float getMililiterFromTime(float durationInMS)
{
  return durationInMS * PUMP_TIME_INMS_FOR_1ML;
}

int getPumpPin(pumpName pump)
{
  switch (pump)
  {
  case A:
    return APUMP_PIN;
    break;

  case B:
    return BPUMP_PIN;
    break;

  case C:
    return CPUMP_PIN;
    break;

  case D:
    return DPUMP_PIN;
    break;

  default:
    return -1;
    break;
  }
}

void setPump(pumpName pump, bool active, bool force = false)
{
  if(pumps[pump] == active && !force) return;
  pumps[pump] = active;
  digitalWrite(2, active ? LOW : HIGH);
  Serial.print("Set pump: ");
  Serial.print(pump);
  Serial.print("to");
  Serial.print(active);
  Serial.print("\n");
}

void resetPumps()
{
  for (int i = 0; i < PUMP_NUMBER; i++)
  {
    setPump(pumpName(i), false, true);
  }
}

bool shouldPourWater(pumpName pump)
{
  return sensorsValues[pump] > MSENSOR_DRY_VALUE;
}

void pourSomeWater(float mililiters, pumpName pump)
{
  if (shouldPourWater(pump))
  {
    Serial.print("Dividing load! \n");
    if(mililiters > PUMP_MAX_VOLUME_ONE_GO_IN_ML)
    {
      mililiters *= .5;
      pourSomeWater(mililiters, pump);
      delay(PUMP_TIME_BETWEEN_POURS_MS);
      pourSomeWater(mililiters, pump);
    }
    else
    {
      Serial.print("Watering plant for:  \n");
      Serial.print(mililiters);
      Serial.print("ml \n");
      setPump(pump, true);
      delay(getDurationFromMililiter(mililiters));
      setPump(pump, false);
    }
  }
}

//Todo assign plant to 
void waterPlants()
{
  pourSomeWater(BASIL_WATER_VOLUME_ML, A);
  pourSomeWater(PEPPER_WATER_VOLUME_ML, B);
  pourSomeWater(MINT_WATER_VOLUME_ML, C);
  pourSomeWater(CIBOUL_WATER_VOLUME_ML, D);
}

void setup() {
  Serial.begin(9600);
  Serial.print("Setup Start \n");
  resetPumps();
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
}

void loop() {
  resetBuffer(sensorsValues);
  readMoistureSensors(sensorsValues, MSENSORS_READ_NUMBER);
  waterPlants();
  delay(MSENSOR_TIME_BETWEEN_READ_MS);
}