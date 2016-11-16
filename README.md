# DataLogger
AVR Data Logger for UBlox GPS, BME280, ADXL345

Logs GPS data at up to 18Hz to a pre-formatted (FAT16/32) SD card.

Log is in binary format and there is an accompanying binary-->CSV program for Windows.

The logger has default parameters (e.g. for log rate) which are stored in the AVR EEPROM.

Parameters can be changed via a menu system on the serial port.  Press ESC twice on the port to enter the menu.