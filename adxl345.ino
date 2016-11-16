/* ========================================================================== */
/*   adxl345.ino                                                              */
/*                                                                            */
/*   i2c code for ADXL345 acclerometer                                        */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* ========================================================================== */

#include <I2C.h>

#define ADXL245_ADDRESS                 0x53
#define ADXL345_REG_DEVID               0x00    // Device ID
#define ADXL345_REG_BW_RATE             0x2C    // Data rate and power mode control
#define ADXL345_REG_POWER_CTL           0x2D    // Power-saving features control
#define ADXL345_REG_DATA_FORMAT         0x31    // Data format control
#define ADXL345_REG_DATA                0x32    // Start of data block (X01, Y01, Z01
#define ADXL345_REG_FIFO_CTL            0x38    // FIFO control
#define ADXL345_REG_FIFO_STATUS         0x39    // FIFO status

void SetupADXL345(void)
{
  unsigned char ID;
  
  I2c.begin();

  // Read device ID
  I2c.read(ADXL245_ADDRESS, ADXL345_REG_DEVID, 1);
  ID = I2c.receive();

  Serial.print(F("ADXL345 ID is ")); Serial.println(ID, HEX);  

  // Set FIFO mode to streaming
  I2c.write(ADXL245_ADDRESS, ADXL345_REG_FIFO_CTL, 0x88);

  // Set data rate
  I2c.write(ADXL245_ADDRESS, ADXL345_REG_BW_RATE, 0x0A);  // 0x08);  A=100Hz, 8=25Hz

  // Start it up
  I2c.write(ADXL245_ADDRESS, ADXL345_REG_POWER_CTL, 0x08);
  
}

int16_t ReadADXL345_16(void)
{
  uint16_t Temp;
  
  Temp = I2c.receive();
  Temp += (I2c.receive() << 8);

  return Temp;
}

void ReadADXL345(void)
{
  I2c.read(ADXL245_ADDRESS, ADXL345_REG_DATA, 6);

  GPS.AccelX = ReadADXL345_16();
  GPS.AccelY = ReadADXL345_16();
  GPS.AccelZ = ReadADXL345_16();
}

