# multifunctionLED
## Technologies
* Atmel Studio
* AVR atmega328p
## Overview
Program is written to be used with Arduino Uno evaluation boars. Microcontroller controls LED diode. Diode is working in three modes: blink mode in which 
it is softly switching on and off, flash mode in which it is toggling on and off and dimming mode in wich LED is dimmed. All three modes can be controlled either by potentiometer or computer via serial monitor.
Current working mode is being displayed on the 2x16 LCD screen as well as blink/flash time interval or dimm percentage. Another info shown by the screen is whether at given time the diode is controlled by computer or potentiometer connected to the microcontroller.

## Sources
LCD_HD44780_IO library was used
