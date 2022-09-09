#include <Arduino.h>
#include <avr/sleep.h>

//Pump pins
const int APUMP_PIN = 2;
const int BPUMP_PIN = 3;
const int CPUMP_PIN = 4;
const int DPUMP_PIN = 5;

//Sensor pins, to pair with pump pins
#define AMOISTURE_SENSOR_PIN A0 // No color
#define BMOISTURE_SENSOR_PIN A1 // Black
#define CMOISTURE_SENSOR_PIN A2 // Red
#define DMOISTURE_SENSOR_PIN A3 // Blue

const int MSENSORS_NUMBER = 4;
const int MSENSORS_READ_NUMBER = 100;
const int MSENSORS_READ_ROUND_NUMBER = 100;
const int MSENSOR_MAX_VALUE = 620; // Dry
const int MSENSOR_MIN_VALUE = 360; //Wet
const float MSENSOR_DRY_PERCENT = .30;
//const float MSENSOR_DRY_VALUE = MSENSOR_MAX_VALUE - ((MSENSOR_MAX_VALUE - MSENSOR_MIN_VALUE)* MSENSOR_DRY_PERCENT);
const float MSENSOR_TIME_BETWEEN_READ_MS = 14400000; //4 hours //43200000 12 hours; //324000; //-> 11 readings per hour //3600000;//1h
const int MSENSORS_TIME_READ_STABILIZE = 30000;

const int PUMP_NUMBER = 4;
const float PUMP_BANDWITH_IN_MLSEC = 21.7;
const float PUMP_TIME_INMS_FOR_1ML = 46.08;
const float PUMP_MAX_VOLUME_ONE_GO_IN_ML = 150;
const float PUMP_TIME_BETWEEN_POURS_MS = 60000;

//Volume of water to pour per plant 
const int DECO_WATER_VOLUME_ML = 200;   //
const int BASIL_WATER_VOLUME_ML = 450;  //
const int MINT_WATER_VOLUME_ML = 450;   //
const int CIBOUL_WATER_VOLUME_ML = 300; //

//Not used, saved for later
// const int PEPPER_WATER_VOLUME_ML = 150;
// const int PERSIL_WATER_VOLUME_ML = 200;
// const int LEMON_WATER_VOLUME_ML = 300;

const int NB_PLANT_PER_PUMP[] = {2, 6, 4, 2};

//sensor values
const int sensorsDryValues[] = {530,515,525,600};
const int sensorsMaxWetValues[] = {400,335,375,364};

enum pumpName {A, B, C, D};
bool pumps[] = {false, false, false, false};
float sensorsValues[] = {0,0,0,0};

int sensorValuesLog[100][MSENSORS_NUMBER];
int logIndex = 0;
bool firstIteration = true;

float getSensorDryValue(int sensor)
{
  float value = sensorsDryValues[sensor] - ((sensorsDryValues[sensor] - sensorsMaxWetValues[sensor])* (1 - MSENSOR_DRY_PERCENT));
  return value;
}

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
  digitalWrite(getPumpPin(pump), active ? LOW : HIGH);
}

void setAllPumps(bool active)
{
  for (int i = 0; i < PUMP_NUMBER; i++)
  {
    setPump(pumpName(i), active, true);
  }
}

void resetPumps()
{
  setAllPumps(false);
}

bool shouldPourWater(pumpName pump)
{
  return sensorsValues[pump] >= getSensorDryValue(pump);
}

void pourSomeWater(float mililiters, pumpName pump)
{
  if (shouldPourWater(pump))
  {
    if(mililiters > (PUMP_MAX_VOLUME_ONE_GO_IN_ML * NB_PLANT_PER_PUMP[pump]))
    {
      mililiters *= .5;
      pourSomeWater(mililiters, pump);
      delay(PUMP_TIME_BETWEEN_POURS_MS);
      pourSomeWater(mililiters, pump);
    }
    else
    {
      setPump(pump, true);
      delay(getDurationFromMililiter(mililiters));
      setPump(pump, false);
    }
  }
}

//Todo assign plant to 
void waterPlants()
{
  pourSomeWater(DECO_WATER_VOLUME_ML * NB_PLANT_PER_PUMP[A], A);
  pourSomeWater(BASIL_WATER_VOLUME_ML * NB_PLANT_PER_PUMP[B], B);
  pourSomeWater(MINT_WATER_VOLUME_ML * NB_PLANT_PER_PUMP[C], C);
  pourSomeWater(CIBOUL_WATER_VOLUME_ML * NB_PLANT_PER_PUMP[D], D);
}

void setPins()
{
  pinMode(APUMP_PIN, OUTPUT);
  pinMode(BPUMP_PIN, OUTPUT);
  pinMode(CPUMP_PIN, OUTPUT);
  pinMode(DPUMP_PIN, OUTPUT);
}

void printAllDrynessValue()
{
  for (int i = 0; i < MSENSORS_NUMBER; i++)
  {
    Serial.print("Dryness level if value of sensor ");
    Serial.print(i);
    Serial.print(" > ");
    Serial.print(getSensorDryValue(i));
    Serial.print(" - Range: ");
    Serial.print(sensorsMaxWetValues[i]);
    Serial.print("-");
    Serial.print(sensorsDryValues[i]);
    Serial.print("\n");
  }
}

void DebugSensors(float *buf)
{
  Serial.print("Debug sensors \n");
  for (int i = 0; i < MSENSORS_NUMBER; i++)
  {
    float sensorDryValue = getSensorDryValue(i);
    Serial.print("Read sensor ");
    Serial.print(i);
    Serial.print(":");
    Serial.print(buf[i]);
    Serial.print(" / ");
    Serial.print(sensorDryValue);
    Serial.print(" Dryness: ");
    float dryness = (buf[i] - sensorsMaxWetValues[i]) / (sensorsDryValues[i] - sensorsMaxWetValues[i]);
    Serial.print(dryness);
    Serial.print("\n");
  }
}

void logValues()
{
  for (int i = 0; i < MSENSORS_NUMBER; i++)
  {
    sensorValuesLog[logIndex][i] = sensorsValues[i];
  }
  logIndex ++;
}

void showLoggedValues()
{
  Serial.print("Logged values \n");
  for (int i = 0; i < logIndex; i++)
  {
    for (int j = 0; j < MSENSORS_NUMBER; j++)
    {
      Serial.print(sensorValuesLog[i][j]);
      Serial.print("  ");
    }
    Serial.print("\n");
  }
  Serial.print("End logged values \n");
}

void readMoistureSensorsWithDelay(float *buf, int numberOfReads)
{
  float tempSensorsValues[] = {0,0,0,0};
  float valuesBuffer[] = {0,0,0,0};

  for (int i = 0; i < numberOfReads; i++)
  {
    readMoistureSensors(tempSensorsValues, MSENSORS_READ_NUMBER);
    for (int j = 0; j < MSENSORS_NUMBER; j++)
    {
      valuesBuffer[j] += tempSensorsValues[j];
    }
    resetBuffer(tempSensorsValues);
    if(i < numberOfReads - 1) delay(MSENSORS_TIME_READ_STABILIZE);
  }
  for (int i = 0; i < MSENSORS_NUMBER; i++)
  {
    buf[i] = valuesBuffer[i] / numberOfReads;
  }
}

void printAllSensors()
{
   Serial.print(analogRead(AMOISTURE_SENSOR_PIN));
   Serial.print(" ");
   Serial.print(analogRead(BMOISTURE_SENSOR_PIN));
   Serial.print(" ");
   Serial.print(analogRead(CMOISTURE_SENSOR_PIN));
   Serial.print(" ");
   Serial.print(analogRead(DMOISTURE_SENSOR_PIN));
   Serial.print("\n");
}

void setup() {
  Serial.begin(9600);
  setPins();
  resetPumps();
  printAllDrynessValue();
}

void loop() {
  //Serial.print("__Start loop__ \n");
  printAllSensors();
  resetBuffer(sensorsValues);
  if(firstIteration) 
  {
    readMoistureSensorsWithDelay(sensorsValues, 1);
    firstIteration = false;
  }
  else readMoistureSensorsWithDelay(sensorsValues, 10);
  // DebugSensors(sensorsValues);
  // logValues();
  // showLoggedValues();
  waterPlants();
  //Serial.print("__Start break__ \n");
  delay(MSENSOR_TIME_BETWEEN_READ_MS);
  //Serial.print("__End break__ \n");
}