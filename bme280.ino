#include <SPI.h>

// Modes
#define BME280_OSAMPLE_1  1
#define BME280_OSAMPLE_2  2
#define BME280_OSAMPLE_4  3
#define BME280_OSAMPLE_8  4
#define BME280_OSAMPLE_16 5

// BME280 Registers
#define BME280_REGISTER_DIG_T1 0x88
#define BME280_REGISTER_DIG_T2 0x8A
#define BME280_REGISTER_DIG_T3 0x8C

#define BME280_REGISTER_DIG_P1 0x8E
#define BME280_REGISTER_DIG_P2 0x90
#define BME280_REGISTER_DIG_P3 0x92
#define BME280_REGISTER_DIG_P4 0x94
#define BME280_REGISTER_DIG_P5 0x96
#define BME280_REGISTER_DIG_P6 0x98
#define BME280_REGISTER_DIG_P7 0x9A
#define BME280_REGISTER_DIG_P8 0x9C
#define BME280_REGISTER_DIG_P9 0x9E

#define BME280_REGISTER_DIG_H1 0xA1
#define BME280_REGISTER_DIG_H2 0xE1
#define BME280_REGISTER_DIG_H3 0xE3
#define BME280_REGISTER_DIG_H4 0xE4
#define BME280_REGISTER_DIG_H5 0xE5
#define BME280_REGISTER_DIG_H6 0xE6
#define BME280_REGISTER_DIG_H7 0xE7

#define BME280_REGISTER_CHIPID 0xD0
#define BME280_REGISTER_VERSION 0xD1
#define BME280_REGISTER_SOFTRESET 0xE0

#define BME280_REGISTER_CONTROL_HUM 0xF2
#define BME280_REGISTER_CONTROL 0xF4
#define BME280_REGISTER_CONFIG 0xF5
#define BME280_REGISTER_PRESSURE_DATA 0xF7
#define BME280_REGISTER_TEMP_DATA 0xFA
#define BME280_REGISTER_HUMIDITY_DATA 0xFD

#define BME_SS 2

struct TBME
{
  // Data registers containing raw readings
  unsigned char Registers[8];
  
  // Calibration Constants
  uint32_t T1;
  int32_t T2, T3;
  uint32_t P1;
  int32_t P2, P3, P4, P5, P6, P7, P8, P9;
  uint8_t H1, H3;
  int16_t H2, H4, H5;
  int8_t H6;
  
  // Raw values from registers
  double RawTemperature;
  double RawPressure;
  double RawHumidity;
  double RawTempFine;
} bme;

void bme280WriteRegister(int Register, int Value)
{  
  // SPI.beginTransaction();
  digitalWrite(BME_SS, 0);

  SPI.transfer(Register & 0x7F);
  SPI.transfer(Value);
  
  digitalWrite(BME_SS, 1);
  // SPI.endTransaction();
}

uint16_t bme280ReadUInt16(int Register)
{
  uint16_t Result;
  
  // SPI.beginTransaction();
  digitalWrite(BME_SS, 0);

  SPI.transfer(Register | 0x80);
  Result = SPI.transfer(0);
  Result |= SPI.transfer(0) << 8;
  // Result = SPI.transfer16(0);
  
  digitalWrite(BME_SS, 1);
  // SPI.endTransaction();

  return Result;
}

uint8_t bme280ReadUInt8(int Register)
{
  uint8_t Result;
  
  // SPI.beginTransaction();
  digitalWrite(BME_SS, 0);

  SPI.transfer(Register | 0x80);
  Result = SPI.transfer(0);
  
  digitalWrite(BME_SS, 1);
  // SPI.endTransaction();

  return Result;
}

int8_t bme280ReadInt8(int Register)
{
  int8_t Result;
  
  // SPI.beginTransaction();
  digitalWrite(BME_SS, 0);

  SPI.transfer(Register | 0x80);
  Result = SPI.transfer(0);
  
  digitalWrite(BME_SS, 1);
  // SPI.endTransaction();

  return Result;
}

int16_t bme280ReadInt16(int Register)
{
  int16_t Result;
  
  // SPI.beginTransaction();
  digitalWrite(BME_SS, 0);

  SPI.transfer(Register | 0x80);
  Result = SPI.transfer(0);
  Result |= SPI.transfer(0) << 8;
  // Result = SPI.transfer16(0);
  
  digitalWrite(BME_SS, 1);
  // SPI.endTransaction();

  return Result;
}

void bme280Calibration(void)
{
  bme.T1 = bme280ReadUInt16(BME280_REGISTER_DIG_T1);
  bme.T2 = bme280ReadInt16(BME280_REGISTER_DIG_T2);
  bme.T3 = bme280ReadInt16(BME280_REGISTER_DIG_T3);

  // Serial.print("T1/2/3 = "); Serial.print(bme.T1); Serial.print(' '); Serial.print(bme.T2); Serial.print(' '); Serial.println(bme.T3);

  bme.P1 = bme280ReadUInt16(BME280_REGISTER_DIG_P1);
  bme.P2 = bme280ReadInt16(BME280_REGISTER_DIG_P2);
  bme.P3 = bme280ReadInt16(BME280_REGISTER_DIG_P3);
  bme.P4 = bme280ReadInt16(BME280_REGISTER_DIG_P4);
  bme.P5 = bme280ReadInt16(BME280_REGISTER_DIG_P5);
  bme.P6 = bme280ReadInt16(BME280_REGISTER_DIG_P6);
  bme.P7 = bme280ReadInt16(BME280_REGISTER_DIG_P7);
  bme.P8 = bme280ReadInt16(BME280_REGISTER_DIG_P8);
  bme.P9 = bme280ReadInt16(BME280_REGISTER_DIG_P9);
  // Serial.print("P1/2/3 = "); Serial.print(bme.P1, HEX); Serial.print(bme.P2, HEX); Serial.println(bme.P3, HEX);
  
  bme.H1 = bme280ReadUInt8(BME280_REGISTER_DIG_H1);
  bme.H2 = bme280ReadInt16(BME280_REGISTER_DIG_H2);
  bme.H3 = bme280ReadUInt8(BME280_REGISTER_DIG_H3);
  // Serial.print("H1/2/3 = "); Serial.print(bme.H1, HEX); Serial.print(bme.H2, HEX); Serial.println(bme.H3, HEX);
  
  bme.H4 = bme280ReadInt8(BME280_REGISTER_DIG_H4);
  bme.H4 *= 16;  // <<= 4;
  bme.H4 |= (bme280ReadUInt8(BME280_REGISTER_DIG_H5) & 0x0F);
//  printf("Registers %d %u, Value %d\n",
//      bme280ReadInt8(BME280_REGISTER_DIG_H4),
//      bme280ReadUInt8(BME280_REGISTER_DIG_H5),
//      bme.H4);
  
  bme.H5 = bme280ReadInt8(BME280_REGISTER_DIG_H6);
  bme.H5 *= 16;  // <<= 4;
  bme.H5 |= (bme280ReadUInt8(BME280_REGISTER_DIG_H5) >> 4);
//  printf("Registers %d %u, Value %d\n",
//      bme280ReadInt8(BME280_REGISTER_DIG_H6),
//      bme280ReadUInt8(BME280_REGISTER_DIG_H5),
//      bme.H5);

  bme.H6 = bme280ReadInt8(BME280_REGISTER_DIG_H7);
  // printf("H6 = %d\n", bme.H6);
}

void bme280StartMeasurement()
{
  int Mode = BME280_OSAMPLE_16;

  bme280WriteRegister(BME280_REGISTER_CONTROL_HUM, Mode);
  
  bme280WriteRegister(BME280_REGISTER_CONTROL, Mode << 5 | Mode << 2 | 1);
}

void bme280ReadDataRegisters(void)
{
  int i;
  
  // SPI.beginTransaction();
  digitalWrite(BME_SS, 0);

  SPI.transfer(BME280_REGISTER_PRESSURE_DATA | 0x80);
  // Serial.print("Raw:");
  for (i=0; i<8; i++)
  {
    bme.Registers[i] = SPI.transfer(0);
    // Serial.print(' ');
    // Serial.print(bme.Registers[i]);
  }
  // Serial.println();
    
  digitalWrite(BME_SS, 1);
  // SPI.endTransaction();

  // printf ("Registers are %02X, %02X ...\n", bme.Registers[0], bme.Registers[1]);
}
  
void bme280GetRawValues(void)
{
  uint32_t high, medium, low, Value;

  // Temperature
  high = bme.Registers[3];
  medium = bme.Registers[4];
  low = bme.Registers[5];
  
  Value = high<<16 | medium<<8 | low;

  bme.RawTemperature = (double)Value / 16.0;
  // Serial.print("Raw temp = "); Serial.println(bme.RawTemperature);
  
  // Pressure
  high = bme.Registers[0];
  medium = bme.Registers[1];
  low = bme.Registers[2];
  
  Value = high<<16 | medium<<8 | low;

  bme.RawPressure = (double)Value / 16.0;
  
  // Raw Humidity
  high = bme.Registers[6];
  low = bme.Registers[7];
  
  Value = high<<8 | low;

  bme.RawHumidity = (double)Value;
}

double bme280Temperature(void)
{
  double var1, var2, T, T1, T2, T3;
    
  T = bme.RawTemperature;

  T1 = bme.T1;
  T2 = bme.T2;
  T3 = bme.T3;
  
  var1 = (T/16384.0 - T1/1024.0) * T2;
  
  var2 = ((T/131072.0 - T1/8192.0) * (T/131072.0 - T1/8192.0)) * T3;
  
  bme.RawTempFine = var1 + var2;

  return (var1 + var2) / 5120.0;
}

double bme280Pressure(void)
{
  double var1, var2, p;

  var1 = (bme.RawTempFine/2.0) - 64000.0;
  var2 = var1 * var1 * ((double)bme.P6) / 32768.0;
  var2 = var2 + var1 * ((double)bme.P5) * 2.0;
  
  var2 = (var2/4.0)+(((double)bme.P4) * 65536.0);
  
  var1 = (((double)bme.P3) * var1 * var1 / 524288.0 + ((double)bme.P2) * var1) / 524288.0;
  
  var1 = (1.0 + var1 / 32768.0)*((double)bme.P1);
  if (var1 == 0.0) return 0;
    
  p = 1048576.0 - bme.RawPressure;
  p = (p - (var2 / 4096.0)) * 6250.0 / var1;
  
  var1 = ((double)bme.P9) * p * p / 2147483648.0;
  var2 = p * ((double)bme.P8) / 32768.0;
  
  p = p + (var1 + var2 + ((double)bme.P7)) / 16.0;
  
  return p / 100;
}

double bme280Humidity(void)
{
  double H;
  
  H = bme.RawTempFine - 76800.0;
  
  H = (bme.RawHumidity - (((double)bme.H4) * 64.0 + ((double)bme.H5) / 16384.0 * H)) *
    (((double)bme.H2) / 65536.0 * (1.0 + ((double)bme.H6) / 67108864.0 * H *
    (1.0 + ((double)bme.H3) / 67108864.0 * H)));
    
  H = H * (1.0 - ((double)bme.H1) * H / 524288.0);
  
  if (H > 100.0) H = 100.0;
  if (H < 0.0) H = 0.0;

  return H;
}

void SetupBME280(void)
{
  pinMode(BME_SS, OUTPUT);
  digitalWrite(BME_SS, 1);
  SPI.begin();

  bme280Calibration();
}

void TriggerBME280(void)
{
  bme280StartMeasurement();
}

void ReadBME280(void)
{
  bme280ReadDataRegisters();
  
  bme280GetRawValues();
  
  GPS.Temperature = bme280Temperature() * 100;
  GPS.Pressure = bme280Pressure() * 10;
  GPS.Humidity = bme280Humidity() * 100;

//  Serial.print(F("Temperature is ")); Serial.println(GPS.Temperature);
//  Serial.print(F("Pressure is ")); Serial.println(GPS.Pressure);
//  Serial.print(F("Humidity is ")); Serial.println(GPS.Humidity);
}

