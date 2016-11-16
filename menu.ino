// Adds a menu accessible via the serial port
// Menu allows the user to change the payload ID etc
// Values are stored in the AVR's flash data memory

#include <EEPROM.h>     // Used for EE storage

int MenuFunction=0;
int LineIndex=0;
int MaxLength=0;
int MinVal, MaxVal;
char Line[32];
typedef enum {ltUNSIGNED, ltSIGNED, ltUPPERDIGITS, ltPRINT, ltBOOLEAN} TLineType;
TLineType LineType;

void EditSetting(int mf, int ml, int from, int to, TLineType lt);

void SetupMenu(void)
{
  if ((EEPROM.read(0) != 'D') || (EEPROM.read(1) != 'A'))
  {
    // Store current (default) settings
    StoreSettings();
  }

  // Load settings from EEPROM
  LoadSettings();

  Serial.println(F("To enter menu, press ESC twice"));
}

void StoreSettings(void)
{
  int i;
  unsigned char *ptr;
  
  // Signature
  EEPROM.write(0, 'D');
  EEPROM.write(1, 'A');

  // Settings
  ptr = (unsigned char *)(&Settings);
  for (i=0; i<sizeof(Settings); i++, ptr++)
  {
    EEPROM.write(i+2, *ptr);
  }
  
  Serial.println(F("EEPROM settings updated"));
}

void EditSetting(int mf, int ml, int from, int to, TLineType lt)
{
  MenuLevel++;
  MenuFunction = mf;
  MaxLength = ml;
  MinVal = from;
  MaxVal = to;
  LineType = lt;
  LineIndex = 0;

  Serial.println();
  Serial.print(F("> "));
}

void LoadSettings(void)
{
  int i;
  unsigned char *ptr;

  ptr = (unsigned char *)(&Settings);
  for (i=0; i<sizeof(Settings); i++, ptr++)
  {
    *ptr = EEPROM.read(i+2);
  }
  
  Serial.println(F("Settings loaded from EEPROM"));
}

void (*RestartProgram)(void) = 0;

void CheckMenu(void)
{
  while (Serial.available())
  {
    char Character;

    Character = Serial.read();

    if (Character == 27)
    {
      // ESC key
      if (MenuLevel < 2)
      {
        MenuLevel++;
        DisplayMenu();
      }
      else if (MenuLevel == 2)
      {
        MenuLevel = 0;
        SetLoggingParameters();
      }
      else
      {
        MenuLevel--;
        DisplayMenu();
      }
    }
    else if (MenuLevel == 2)
    {
      switch (Character)
      {
        case '0':
          LoadDefaults();
        break;
        
        case '1': 
          Serial.print(F("Logging Period = "));
          Serial.print(Settings.LoggingPeriod);
          Serial.println("ms");
          Serial.println(F("Please enter new value 50-1000 (ms) then ENTER to save or ESC to cancel"));
          EditSetting(1, 4, 50, 1000, ltUNSIGNED);
        break;
        
        case '2': 
          Serial.print(F("Log Filename Prefix = '"));
          Serial.print(Settings.FilenamePrefix);
          Serial.println("'");
          Serial.println(F("Please enter new value 1-7 characters then ENTER to save or ESC to cancel"));
          EditSetting(2, 7, 1, 7, ltPRINT);
        break;
        
        case '3': 
          Serial.print(F("Log Filename Digits = "));
          Serial.print(Settings.FilenameDigits);
          Serial.println();
          Serial.println(F("Please enter new value from 1-7 then ENTER to save or ESC to cancel"));
          EditSetting(3, 1, 1, 7, ltUNSIGNED);
        break;
        
        case '4': 
          Serial.print(F("Log Filename Size = "));
          Serial.print(Settings.LogFileSize);
          Serial.println("MB");
          Serial.println(F("Please enter new value 1-256 (MB) then ENTER to save or ESC to cancel"));
          EditSetting(4, 3, 1, 256, ltUNSIGNED);
        break;
        
        case '5': 
          Serial.print(F("Flight Mode Altitude = "));
          Serial.print(Settings.FlightModeAltitude);
          Serial.println("m");
          Serial.println(F("Please enter new value 0-9999 (m) then ENTER to save or ESC to cancel"));
          EditSetting(5, 4, 0, 9999, ltUNSIGNED);
        break;
        
        case '6': 
          Serial.print(F("PowerSavingEnabled = "));
          Serial.print(Settings.PowerSavingEnabled ? "Yes" : "No");
          Serial.println("'");
          Serial.println(F("Please enter new value Y/N then ENTER to save or ESC to cancel"));
          EditSetting(6, 1, 0, 1, ltBOOLEAN);
        break;
        
        case '9':
          RestartProgram();
        break;    
      }
      DisplayMenu();
    }
    else if (MenuLevel == 3)
    {
      // User typing in a value
      if (Character == 8)
      {
        // backspace
        if (LineIndex)
        {
          LineIndex--;
          Serial.print(Character);
          Serial.print(' ');
          Serial.print(Character);
        }
      }
      else if (isprint(Character) && (LineIndex < MaxLength))
      {
        int OK=0;

        if (LineType == ltUNSIGNED) OK = isdigit(Character);
        if (LineType == ltSIGNED) OK = isdigit(Character) || (Character == '-');
        if (LineType == ltUPPERDIGITS) OK = isdigit(Character) || isupper(Character);
        if (LineType == ltPRINT) OK = 1;
        if (LineType == ltBOOLEAN) OK = (strchr("YyNn", Character) != NULL);

        if (OK)
        {
          Line[LineIndex++] = Character;
          Line[LineIndex] = '\0';
          Serial.print(Character);
        }
      }
      else if (Character == 13)
      {
        // CR
        Serial.println();
        Serial.println();
        if (LineIndex > 0)
        {
          // Save setting
          Line[LineIndex] = '\0';
          switch (MenuFunction)
          {
              case 1:
              Settings.LoggingPeriod = ReadInteger(Line, MinVal, MaxVal);
              break;
              
              case 2:
              strcpy(Settings.FilenamePrefix, Line);
              break;
              
              case 3:
              Settings.FilenameDigits = ReadInteger(Line, MinVal, MaxVal);            
              break;

              case 4:
              Settings.LogFileSize = ReadInteger(Line, MinVal, MaxVal);
              break;
              
              case 5:
              Settings.FlightModeAltitude = ReadInteger(Line, MinVal, MaxVal);
              break;
              
              case 6:
              Settings.PowerSavingEnabled = *Line == 'Y' || *Line == 'y';
              break;              
          }
          StoreSettings();
          Serial.println();
          Serial.println();
          Serial.println(F("SAVED !!"));
        }
        MenuLevel--;
        DisplayMenu();
      }
    }
  }
}

int ReadInteger(char *Line, int MinVal, int MaxVal)
{
  int Temp;

  Temp = atoi(Line);
  if (Temp < MinVal) Temp = MinVal;
  if (Temp > MaxVal) Temp = MaxVal;

  return Temp;
}

void DisplayMenu(void)
{ 
  if (MenuLevel == 1)
  {
    Serial.println(F("Press ESC again to enter main menu"));
  }
  else if (MenuLevel == 2)
  {
    Serial.println();
    Serial.println();
    Serial.println(F("  LOGGER MENU"));
    Serial.println(F("  ==========="));
    Serial.println();
    Serial.println(F("  0 - Reset to defaults"));  
    
    Serial.print(F("  1 - Logging period ("));        Serial.print(Settings.LoggingPeriod);                         Serial.println("ms)");
    Serial.print(F("  2 - Log Filename Prefix ("));   Serial.print(Settings.FilenamePrefix);                        Serial.println(")");
    Serial.print(F("  3 - Log Filename Digits ("));   Serial.print(Settings.FilenameDigits);                        Serial.println(")");   
    Serial.print(F("  4 - Log File Size: ("));        Serial.print(Settings.LogFileSize);                           Serial.println("MB)");
    Serial.print(F("  5 - Flight Mode Altitude ("));  Serial.print(Settings.FlightModeAltitude);                    Serial.println("ms)");
    Serial.print(F("  6 - Power Saving Enabled ("));  Serial.print(Settings.PowerSavingEnabled ? "Yes" : "No");     Serial.println(")");  
    
    Serial.println(F("  9 - Restart Logger"));
    
    Serial.println(F("ESC - Exit Menu"));
    
    Serial.println();
    Serial.print("> ");
  }
}


