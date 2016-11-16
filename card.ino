// SD Card Storage

SdFat sd;
SdBaseFile DataFile;
static uint32_t FileBlockCount;
uint32_t bgnBlock, endBlock, curBlock;

void SetupCard(void)
{
  Serial.print(F("Initializing SD card..."));

  if (!sd.begin(SD_CS))
  {
    ShowError(1, F("SD Card Missing!"));
  }

  Serial.println();
  Serial.println(F("Card initialised OK"));
}

void OpenLogFile(void)
{
  uint32_t const ERASE_SIZE = 262144L;
  uint32_t i, j;
  char format[12];
  char FileName[16];
  uint32_t bgnErase;
  uint32_t endErase;
  int Flash = 0;

  FileBlockCount = (uint32_t)(Settings.LogFileSize) * 2048L;
  Serial.print(F("FileBlockCount=")); Serial.println(FileBlockCount);

  // Get filename
  sprintf(format, "%s%%0%dd.bin", Settings.FilenamePrefix, Settings.FilenameDigits);
  j = 1;
  for (i = 0; i <= Settings.FilenameDigits; i++)
  {
    j *= 10;
  }
  for (i = 0; i < j; i++)
  {
    sprintf(FileName, format, i);
    if (!sd.exists(FileName))
    {
      break;
    }
  }

  Serial.print(F("Creating and clearing "));
  Serial.println(FileName);

  if (!DataFile.createContiguous(sd.vwd(), FileName, (uint32_t)512 * FileBlockCount))
  {
    // Failed
    ShowError(2, F("SD Card File Init Failure - createContiguous() Failed"));
  }

  // Get the address of the file on the SD.
  Serial.println(F("Getting start and end of file"));
  if (!DataFile.contiguousRange(&bgnBlock, &endBlock))
  {
    // Failed
    ShowError(3, F("SD Card File Init Failure - contiguousRange() Failed"));
  }

  Serial.print(F("Blocks from ")); Serial.print(bgnBlock); Serial.print(F(" to ")); Serial.println(endBlock);

  // Use SdFat's internal buffer.
  Serial.println(F("Getting cache"));
  SDBuffer = (uint8_t *)(sd.vol()->cacheClear());
  if (SDBuffer == 0)
  {
    // Failed
    ShowError(4, F("SD Card File Init Failure - cacheClear() Failed"));
  }

  Serial.print(F("Cache =  ")); Serial.println((uint16_t)SDBuffer);

  // Flash erase all data in the file.
  Serial.print(F("Erasing all data"));
  bgnErase = bgnBlock;
  while (bgnErase < endBlock)
  {
    Serial.print('.');
    ControlLEDs(Flash = !Flash, 1);
    endErase = bgnErase + ERASE_SIZE;
    if (endErase > endBlock)
    {
      endErase = endBlock;
    }

    if (!sd.card()->erase(bgnErase, endErase))
    {
      // Failed
      ShowError(5, F("SD Card File Init Failure - erase() Failed"));
    }
    bgnErase = endErase + 1;
  }
  Serial.println();

  /* 
  if (!sd.card()->writeStart(bgnBlock, FileBlockCount))
  {
    ShowError(5, F("SD Card Init Failure - writeStart() Failed"));
  }
  */

  curBlock = bgnBlock;
}

int LogData(void)
{
  Serial.print(F("LogData() ")); Serial.print(SDBuffer[0], HEX); Serial.print(' '); Serial.print(SDBuffer[1], HEX); Serial.print(' '); Serial.println(SDBuffer[2], HEX);

  if (!sd.card()->writeStart(curBlock, 1))
  {
    Serial.println(F("Failed to start block write"));
  }
  
  if (!sd.card()->writeData(SDBuffer))
  {
    Serial.println(F("Failed to write block"));
  }
  
  sd.card()->writeStop();

  curBlock++;

  if (curBlock >= endBlock)
  {
    DataFile.close();
    OpenLogFile();
  }

  return 1;
}


