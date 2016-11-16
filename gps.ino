/* ========================================================================== */
/*   gps.ino                                                                  */
/*                                                                            */
/*   Serial and i2c code for ublox on AVR                                     */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* ========================================================================== */

// Include i2c library, if using it

#include <I2C.h>

// Globals
byte RequiredFlightMode=0;
byte CurrentFlightMode=0;
byte GlonassMode=0;
byte RequiredPowerMode=-1;
byte LastCommand1=0;
byte LastCommand2=0;
byte HaveHadALock=0;
byte FlightMode=0;
byte CurrentPowerMode=0;

char Hex(char Character)
{
  if (Character >= 10)
  {
      return 'A' + Character - 10;
  }
  else
  {
    return '0' + Character;
  }
}

void FixUBXChecksum(unsigned char *Message, int Length)
{ 
  int i;
  unsigned char CK_A, CK_B;
  
  CK_A = 0;
  CK_B = 0;

  for (i=2; i<(Length-2); i++)
  {
    CK_A = CK_A + Message[i];
    CK_B = CK_B + CK_A;
  }
  
  Message[Length-2] = CK_A;
  Message[Length-1] = CK_B;
}

void SendUBX(unsigned char *Message, int Length)
{
  LastCommand1 = Message[2];
  LastCommand2 = Message[3];
  
  I2c.write(0x42, 0, Message, Length);
}


void DisableNMEAProtocol(unsigned char Protocol)
{
  unsigned char Disable[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  
  Disable[7] = Protocol;
  
  FixUBXChecksum(Disable, sizeof(Disable));
  
  SendUBX(Disable, sizeof(Disable));
  
  // if (!MenuLevel) Serial.print("Disable NMEA "); Serial.println(Protocol);
}

void SetFlightMode(byte NewMode)
{
  // Send navigation configuration command
  unsigned char setNav[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC};

  setNav[8] = NewMode;

  FixUBXChecksum(setNav, sizeof(setNav));
  
  SendUBX(setNav, sizeof(setNav));
}

    
void SetGNSSMode(void)
 {
  // Sets CFG-GNSS to disable everything other than GPS GNSS
  // solution. Failure to do this means GPS power saving 
  // doesn't work. Not needed for MAX7, needed for MAX8's
  
  uint8_t setGNSS[] = {
    0xB5, 0x62, 0x06, 0x3E, 0x2C, 0x00, 0x00, 0x00,
    0x20, 0x05, 0x00, 0x08, 0x10, 0x00, 0x01, 0x00,
    0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x03, 0x08, 0x10, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x06, 0x08, 0x0E, 0x00, 0x00, 0x00,
    0x01, 0x01, 0xFC, 0x11};
    
  SendUBX(setGNSS, sizeof(setGNSS));
} 

void SetPowerMode(byte SavePower)
{
  uint8_t setPSM[] = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 };
  
  setPSM[7] = SavePower ? 1 : 0;
  
  FixUBXChecksum(setPSM, sizeof(setPSM));
  
  SendUBX(setPSM, sizeof(setPSM));
}


void ProcessUBX_ACK(unsigned char *Buffer, int Length)
{
  if ((LastCommand1 == 0x06) && (LastCommand2 == 0x24))
  {
    FlightMode = RequiredFlightMode;
  }
  else if ((LastCommand1 == 0x06) && (LastCommand2 == 0x3E))
  {
    GlonassMode = 1;
  }
  else if ((LastCommand1 == 0x06) && (LastCommand2 == 0x11))
  {
    CurrentPowerMode = RequiredPowerMode;
  }
  LastCommand1 = 0;
  LastCommand2 = 0;
}

void ProcessUBX_NAV_PVT(unsigned char *Buffer, int Length)
{
  struct TUBlox
  {
    uint32_t Time;
    uint16_t Year;
    uint8_t Month;
    uint8_t Day;
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
    uint8_t Valid;
    uint32_t TimeAccuracy;
    int32_t NanoSeconds;
    uint8_t FixType;
    uint8_t Flags;
    uint8_t Reserved;
    uint8_t Satellites;
    int32_t Longitude;
    int32_t Latitude;
    int32_t HeightEllipsoid;
    int32_t HeightSeaLevel;
    uint32_t HAccuracy;
    uint32_t VAccuracy;
  } *UBlox;
    
  UBlox = (struct TUBlox*)(Buffer+6);

  GPS.FixType = UBlox->FixType;
  GPS.Satellites = UBlox->Satellites;
  
  if (UBlox->FixType > 0)
  {
    GPS.Year = UBlox->Year - 2000;
    GPS.Month = UBlox->Month;
    GPS.Day = UBlox->Day;
    GPS.MilliSeconds = UBlox->Time % 1000L;
    // GPS.SecondsInDay = (UBlox->Time / 1000) % 86400;      // Time of day in seconds = Time in week in ms / 1000, mod 86400
    GPS.Hours = UBlox->Hours; // GPS.SecondsInDay / 3600;
    GPS.Minutes = UBlox->Minutes; // (GPS.SecondsInDay / 60) % 60;
    GPS.Seconds = UBlox->Seconds; // GPS.SecondsInDay % 60;  
    
    if ((UBlox->FixType >= 1) && (UBlox->FixType <= 4))
    {
      GPS.Longitude = UBlox->Longitude;
      GPS.Latitude = UBlox->Latitude;
      
      if ((UBlox->FixType >= 3) && (UBlox->FixType <= 4))
      {
        GPS.Altitude = UBlox->HeightSeaLevel / 1000;
      }
    }
  }
}

void ProcessUBX(unsigned char *Buffer, int Length)
{
  // Serial.print("UBX "); Serial.print(Buffer[2], HEX);
  // Serial.print(" "); Serial.println(Buffer[3], HEX);
  
  if ((Buffer[2] == 0x05) && (Buffer[3] == 0x01))
  {
    ProcessUBX_ACK(Buffer, Length);
  }
  else if ((Buffer[2] == 1) && (Buffer[3] == 7))
  {
    ProcessUBX_NAV_PVT(Buffer, Length);
  }
}

void ProcessNMEA(unsigned char *Buffer, int Count)
{
    if (strncmp((char *)Buffer+3, "GGA", 3) == 0)
    {
      DisableNMEAProtocol(0);      
    }
    else if (strncmp((char *)Buffer+3, "RMC", 3) == 0)
    {
      DisableNMEAProtocol(4);
    }
    else if (strncmp((char *)Buffer+3, "GSV", 3) == 0)
    {
      DisableNMEAProtocol(3);
    }
    else if (strncmp((char *)Buffer+3, "GLL", 3) == 0)
    {
      DisableNMEAProtocol(1);
    }
    else if (strncmp((char *)Buffer+3, "GSA", 3) == 0)
    {
      DisableNMEAProtocol(2);
    }
    else if (strncmp((char *)Buffer+3, "VTG", 3) == 0)
    {
      DisableNMEAProtocol(5);
    }
}


void SetupGPS(void)
{
  I2c.begin();
}

int GPSAvailable(void)
{
  int FD, FE, Bytes;
  
  I2c.read(0x42, 0xFD, 2);    // Set up read from registers FD/FE - number of bytes available
  FD = I2c.receive();
  FE = I2c.receive();
  
  Bytes = FD * 256 + FE;

  if (Bytes > 32)
  {
    Bytes = 32;
  }
  
  if (Bytes > 0)
  {
    I2c.read(0x42, 0xFF, Bytes);    // request up to 32 bytes from slave device 0x42 (Ublox default)
  }

  return Bytes;
}

char ReadGPS(void)
{
  return I2c.receive();        
}

void SetGPSRate(unsigned int MilliSeconds)
{
  uint8_t request[] = {0xB5, 0x62,                            // Header
                       0x06, 0x08,                            // Class, ID
                       0x06, 0x00,                            // Payload length
                       0x00, 0x00, 0x01, 0x00, 0x00, 0x00,    // Payload
                       0x00, 0x00};                           // Checksum

  // Fill in rate
  request[6] = (unsigned char)(MilliSeconds & 0xFF);
  request[7] = (unsigned char)(MilliSeconds >> 8);
  
  FixUBXChecksum(request, sizeof(request));

  SendUBX(request, sizeof(request));

  WaitForGPSReply();
}

void PollGPSPosition(void)
{
  // uint8_t request[] = {0xB5, 0x62, 0x01, 0x02, 0x00, 0x00, 0x03, 0x0A};  // NAV-POSLLH
  uint8_t request[] = {0xB5, 0x62, 0x01, 0x07, 0x00, 0x00, 0x08, 0x19};
  
  SendUBX(request, sizeof(request));
}

int SetGPSFlightMode(void)
{
  int RequiredFlightMode;
  
  RequiredFlightMode = (GPS.Altitude > Settings.FlightModeAltitude) ? 6 : 3;    // 6 is airborne <1g mode; 3=Pedestrian mode
  
  if (RequiredFlightMode != CurrentFlightMode)
  {
    SetFlightMode(RequiredFlightMode);
    return 1;
  }  
  return 0;
}

int SetGPSPowerSaving(void)
{
  int RequiredPowerMode;

  RequiredPowerMode = (GPS.FixType==3) && (GPS.Satellites>=5) && Settings.PowerSavingEnabled;
      
  if (RequiredPowerMode != CurrentPowerMode)
  {
    SetPowerMode(RequiredPowerMode);
    return 1;
  }

  return 0;
}
 
/*     
  // Power saving
  #ifdef POWERSAVING
    if (!GlonassMode)
    {
      if (!MenuLevel) Serial.println("*** SetGNSSMode() ***");
      SetGNSSMode();
    }
}
*/

int CheckGPS(void)
{
  static unsigned char Line[128];
  static int Length=0;
  static int UBXLength=0;
  static unsigned int PollMode=0;
  unsigned char Character, Bytes, i, GotReply;
  
  Length = 0;
  GotReply = 0;

  do
  {
    Bytes = GPSAvailable();
  
    for (i=0; i<Bytes; i++)
    { 
      Character = ReadGPS();
    
      if ((Character == 0xB5) && (Length <= 4))
      {
        Line[0] = Character;
        Length = 1;
      }
      else if ((Character == 0x62) && (Length <= 4))
      {
        if (Length == 0)
        {
          Serial.print("MISSED B5 ");
          Line[0] = 0xB5;
        }
        Line[1] = Character;
        Length = 2;
      }
      else if ((Character == '$') && (Length == 0))
      {
        Line[0] = Character;
        Length = 1;
      }
      else if (Length >= (sizeof(Line)-2))
      {
        Length = 0;
      }
      else if (Length > 0)
      {
        if (Line[0] == 0xB5)
        {
          // UBX
          Line[Length++] = Character;
          if (Length == 6)
          {
            // Got UBX Length
            UBXLength = (int)Line[4] + (int)(Line[5]) * 256;
          }
          else if ((Length > 6) && (Length >= (UBXLength+8)))
          {
            ProcessUBX(Line, Length);
            
            LastCommand1 = Line[2];
            LastCommand2 = Line[3];
            
            Length = 0;
            GotReply = 1;
          }
        }
        else if (Line[0] == '$')
        {
          if (Character == '$')
          {
            Length = 1;
          }
          else if (Character != '\r')
          {
            Line[Length++] = Character;
            if (Character == '\n')
            {
              Line[Length] = '\0';
              ProcessNMEA(Line, Length);
              Length = 0;
            }
          }
        }
      }
    }
  } while (Bytes > 0);

  return GotReply;
}

void WaitForGPSReply(void)
{
  int GPS_Reply;

  for (GPS_Reply=0; !GPS_Reply; )
  {
    GPS_Reply = CheckGPS();
    CheckMenu();
  }
}

