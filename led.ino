void SetupLEDs(void)
{
  pinMode(LED_OK, OUTPUT);
  digitalWrite(LED_OK, 0);
  pinMode(LED_WARN, OUTPUT);
  digitalWrite(LED_WARN, 0);
}

void ControlLEDs(int LEDOK, int LEDWarn)
{
  digitalWrite(LED_OK, LEDOK);
  digitalWrite(LED_WARN, LEDWarn);
}


