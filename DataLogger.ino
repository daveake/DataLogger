#include <SdFat.h>

#include <SPI.h>
#include <avr/pgmspace.h>

/*------------------------------------------------------------------------------------------------------*\
|                                                                                                        |
| SD GPS Data Logger                                                                                     |
|                                                                                                        |
|                                                                                                        |
| V0.00   First stab                                                                                     |
|                                                                                                        |
\*------------------------------------------------------------------------------------------------------*/


//------------------------------------------------------------------------------------------------------

#define BUFFER_SIZE 512

// HARDWARE DEFINITION

#define LED_OK                  6
#define LED_WARN                5
#define SD_CS                  10
#define DEF_LOG_PERIOD       1000
#define BATTERY_MULTIPLIER   1535
#define BATTERY_DIVIDER       475

//------------------------------------------------------------------------------------------------------

//  Globals

struct TSettings
{
  char FilenamePrefix[9];
  int FilenameDigits;
  unsigned int LoggingPeriod;
  unsigned int LogFileSize;
  long FlightModeAltitude;
  int PowerSavingEnabled;
} Settings;

struct TGPS
{
  // Signature 1 byte
  uint8_t Signature;

  // Version 1 byte
  uint8_t Version;

  // Fix type 1 byte
  uint8_t FixType;

  // Date and time 8 bytes
  uint8_t Year, Month, Day;
  uint8_t Hours, Minutes, Seconds;
  uint16_t MilliSeconds;

  // Position 12 bytes
  int32_t Longitude, Latitude;
  int32_t Altitude;

  // Misc GPS 1 byte
  uint8_t Satellites;

  // ADC 2 bytes
  uint16_t BatteryVoltage;

  // Accelerometer 6 bytes
  int16_t AccelX, AccelY, AccelZ;

  // BME280 6 bytes
  int16_t Temperature, Pressure, Humidity;
} GPS;  // Total 1+1+1+8+12+1+2+6+6 = 38 bytes

int MenuLevel = 0;      // 0 = awaiting 2 ESCAPES to enter
                        // 1 = awaiting second ESCAPE to enter
                        // 2 = top level menu
                        // 3 = module menu (e.g. APRS)

uint8_t *SDBuffer;

//------------------------------------------------------------------------------------------------------

void setup()
{
  int i;
  
  SetupLEDs();
  
  LoadDefaults();
  
  Serial.begin(57600);
  Serial.println();
  Serial.println(F("Data Logger V1.0"));
  Serial.println();

  Serial.print(F("Free memory = "));
  Serial.println(freeRam());

  // Flash LEDs for startup delay
  Serial.print(F("Starting"));
  for (i=0; i<20; i++)
  {
    Serial.print('.');
    ControlLEDs(0,1);
    delay(50);  
    ControlLEDs(1,0);
    delay(50);  
  }
  Serial.println();

  ControlLEDs(1,1);

  SetupMenu();
  
  SetupCard();
  
  OpenLogFile();

  ControlLEDs(0,0);
  
  SetupGPS();
  
  SetupADC();

  SetupADXL345();
  
  SetupBME280();
/*  
  if (useSharedSpi) {
          sd.card()->chipSelectHigh();
        }
        acquireData(&curBlock->data[curBlock->count++]);
        if (useSharedSpi) {
          sd.card()->chipSelectLow();
        }        */

  // OK off Warn on
  ControlLEDs(0,1);

  SetLoggingParameters();
}


void SetLoggingParameters(void)
{
  SetGPSRate(Settings.LoggingPeriod);
}

void loop()
{
  static unsigned long BufferIndex=0;
  char Temp[80];
  
  ControlLEDs(0, !GPS.FixType);

  // Trigger anything that is slow
  TriggerBME280();
 
  // Loop 1 - GPS Position
  PollGPSPosition();
  // Sensors now, as this command is slow to respond (it waits according to the GPS update rate)
  ReadADC();
  ReadADXL345();
  // Now get reply from GPS
  WaitForGPSReply();

  // Loop 2 - GPS Flight Mode
  if (SetGPSFlightMode())
  {
    WaitForGPSReply();
  }

  // Loop 3 - GPS Power saving 
  if (SetGPSPowerSaving())
  {
    WaitForGPSReply();
  }

  // Now read sensors that we triggered earlier
  ReadBME280();
  
  ControlLEDs(1, !GPS.FixType);
                                                            
  // Log to serial port
  if (!MenuLevel)
  {
    // Build line to send
    sprintf(Temp, "%02d:%02d:%02d.%04d,", GPS.Hours, GPS.Minutes, GPS.Seconds, GPS.MilliSeconds);
    Serial.print(Temp);
    
    Serial.print((float)GPS.Latitude / 10000000, 5); Serial.print(',');
    Serial.print((float)GPS.Longitude / 10000000, 5); Serial.print(',');
    Serial.print(GPS.Altitude); Serial.print(',');
    Serial.print((float)GPS.BatteryVoltage / 1000, 2); Serial.print(',');
    Serial.print((float)GPS.AccelX / 250, 3);  Serial.print(',');
    Serial.print((float)GPS.AccelY / 250, 3); Serial.print(',');
    Serial.print((float)GPS.AccelZ / 250, 3); Serial.print(',');
    Serial.print((float)GPS.Temperature / 100, 2); Serial.print(',');
    Serial.print((float)GPS.Pressure / 10, 1); Serial.print(',');
    Serial.print((float)GPS.Humidity / 100, 2);
    Serial.println();
  }

  GPS.Signature = 0xA5;
  GPS.Version = 0x01;
  memcpy((void *)(SDBuffer+BufferIndex), (void *)&GPS, sizeof(GPS));
  BufferIndex += sizeof(GPS);
  if ((BufferIndex + sizeof(GPS)) >= BUFFER_SIZE)
  {
    // Log to SD card 
    Serial.print(F("Writing block ")); Serial.print(SDBuffer[0], HEX); Serial.print(' '); Serial.print(SDBuffer[1], HEX); Serial.print(' '); Serial.println(SDBuffer[2], HEX);
    LogData();
    BufferIndex = 0;
  }
}

void ShowError(int ErrorCode, const __FlashStringHelper *Message)
{
  while (1)
  {
    int i;
    
    Serial.println(Message);
    for (i=0; i<ErrorCode; i++)
    {
      ControlLEDs(0,1);
      delay(100);  
      ControlLEDs(0,0);
      delay(100);  
      ControlLEDs(0,0);
    }
    delay(1000);
  }
}


int freeRam(void)
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void LoadDefaults(void)
{
  strcpy(Settings.FilenamePrefix, "dlog");
  Settings.FilenameDigits = 4;
  Settings.LogFileSize = 64;

  Settings.LoggingPeriod = DEF_LOG_PERIOD;

  Settings.FlightModeAltitude = 2000;
  Settings.PowerSavingEnabled = 0;
}

