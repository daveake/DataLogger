/* ========================================================================== */
/*   adc.ino                                                                  */
/*                                                                            */
/*   Code for reading/averaging ADC channels                                  */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* ========================================================================== */

void SetupADC(void)
{
  analogReference(DEFAULT);
  pinMode(A0, INPUT);
}

void ReadADC(void)
{
  unsigned long Temp;
  
  Temp = analogRead(A0);
  Temp *= BATTERY_MULTIPLIER;
  Temp /= BATTERY_DIVIDER;

  GPS.BatteryVoltage = Temp;
}

