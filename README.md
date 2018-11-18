# catfeeder
Uses a cereal dispenser, Arduino, and servos to dispense food to a cat (or dog....if you must...)

## Bill of Materials:

Item                                         |QTY   | URL                                      |
---------------------------------------------|:----:|------------------------------------------|
Adafruit Metro (or other AMTEL 328 microcontroller board) | 1    | https://www.adafruit.com/product/2488 
16x2 LCD Display Module                      | 1    | https://www.adafruit.com/product/181
MP23017                                      | 1    | https://www.adafruit.com/product/732
24LC256                                      | 1    | 
Arduino Enclosure                            | 1    | https://www.adafruit.com/product/337
Cereal Dispensor (with rotational dispensing mechanisim) | 1 | 
Micro Servo | 2 | https://www.adafruit.com/product/169
PCF8523 (breakout board) | 1 | https://www.adafruit.com/product/3295


## LCD Display to MCP23017 Wire Map

LCD Pin | MCP23017 Pin |
:------:|:------------:|
4 | 8
5 | 7
6 | 6
7 | NC
8 | NC
9 | NC
10 | NC
11 | 5
12 | 4
13 | 3
14 | 2

## MCP23017 Pinout (I2C Address: 0x50)

L Pin | Assignment | R Pin | Assignment
------|------------|-------|-----------
1 | NC | 28 | NC 
2 | LCD pin 14 | 27 | LCD pin 15
3 | LCD pin 13 | 26 | NC
4 | LCD pin 12 | 25 | Button Left
5 | LCD pin 11 | 24 | Button Up
6 | LCD pin 6 | 23 | Button Down
7 | LCD pin 5 | 22 | Button Right
8 | LCD pin 4 | 21 | Button Select
9 | 5V | 20 | NC
10 | Ground | 19 | NC
11 | NC | 18 | 5V
12 | Arduino A5 | 17 | Ground
13 | Arduino A4 | 16 | Ground
14 | NC | 15 | Ground

## 24LC256 Pinout (I2C Address: 0x51)

L Pin | Assignment | R Pin | Assignment
------|------------|-------|-----------
1 | 5V | 8 | 5V
2 | Ground | 7 | Ground
3 | Ground | 6 | Arduino A5
4 | Grounf | 5 | Arduino A4

## Menus

The menu is opened by pressing select button. This the triggers a loop that times-out upon reaching so many seconds. Pressing select on a menu items opens a setting or a sub-menu. After changing the setting with the left and right buttons, pressing select saves the new paremter to the variable and stores it in the 24LC256 eeprom.