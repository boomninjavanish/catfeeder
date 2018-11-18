#include <Servo.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include "RTClib.h"
#include "eepromi2c.h"

/*
  TODO: 
    
    make function to detect and deny invalid dates from being put to RTC

      */

// configuration that is stored in eeprom
struct config
{
	int feedingTimeHours[2]; // feeding hours
	int feedingTimeMinutes[2]; // feeding minutes
	int defautRotationTime; // time in milliseconds to rotate servos
	int offset; // amount of hours to offset to display clock in proper time zone
	int eepromHasConfig; // variable to test if the eeprom is new
	} config;

// pin configuration
const int leftServoPin = 11;
const int rightServoPin = 10;
const int leftLightPin = 5;
const int rightLightPin = 4;
const int beeperPin = 6;

// servo
Servo chosenServo;

// lcd
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// Real time clock
RTC_DS1307 RTC;

void setup() {
  // read config from eeprom
  Wire.begin();
  RTC.begin();
  eeRead(0, config);
  delay(50);
  // test to see if eeprom is new and has no config
  // if eepromHasConfig equals 42, then load the config stored in eeprom
  // otherwise, use default config  and load it to eeprom.
  if (config.eepromHasConfig != 42){
  	config.feedingTimeHours[0] = 6;
  	config.feedingTimeHours[1] = 18;
  	config.feedingTimeMinutes[0] = 30;
  	config.feedingTimeMinutes[1] = 30; 	
  	config.defautRotationTime = 750;
  	config.offset = -4;
  	config.eepromHasConfig = 42;
  	eeWrite(0, config);
  	delay(50);
  }
  
  // setup pin modes
  pinMode(leftLightPin, OUTPUT);
  pinMode(rightLightPin, OUTPUT);
  pinMode(beeperPin, OUTPUT);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  
  lcd.print("Cat Feeder 9000 ");

  if (! RTC.isrunning()) {
  	beep(10);
  	delay(20);
    // following line sets the RTC to the date & time when this sketch is compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
    beep(10);
    delay(20);
}

  // beep to let us know it has finished setup
  beep(100);

}

void loop() {
  // gps clock update and display
  updateClock();
  
  // read buttons
  checkButtons();

  // check for feeding time
  checkFeedingTime();  
}


// moveServo moves a specified servo forward and lights a specified LED for the time that is 
// stored in the config in eeprom
// servoPin = pass the left or right servo pin var
// ledPin = pass the left or right LED pin
void moveServo(int servoPin, int ledPin){
	int servoPulse = 2000;
	int stopServo = 1500;

	if (servoPin == leftServoPin){
		drawDisplayBottom("Feeding (Left)  ");
	}
	else {
		drawDisplayBottom("Feeding (Right) ");
	}

	chosenServo.attach(servoPin);
	digitalWrite(ledPin, HIGH);
	chosenServo.writeMicroseconds(servoPulse);
	delay(config.defautRotationTime);
	chosenServo.writeMicroseconds(stopServo);
	chosenServo.detach();
	delay(20);
	digitalWrite(ledPin, LOW);
}

// checkButtons runs as part of the main loop and constantly polls the buttons for presses
// if the left or right buttons are pressed, the corresponding servo will turn and LED will light
// if the select button is pressed, the enterMenu function is initiated allowing a user to adjust settings
void checkButtons() 
{
	uint8_t navButtons = lcd.readButtons();

  // check feeding buttons then rotate servo if pressed
  if (navButtons & BUTTON_LEFT) {
  	beep(100);
  	moveServo(leftServoPin, leftLightPin);
  	beep(50);
  }
  if (navButtons & BUTTON_RIGHT) {
  	beep(100);
  	moveServo(rightServoPin, rightLightPin);
  	beep(50);
  }
  
  // check for enter button being pressed, if pressed, go into menu loop
  if (navButtons & BUTTON_SELECT)
  {
  	enterSettingsMenu();
  }
}

// beep beeps the piezo speaker for a specified amount of time 
// beepLength is the legth of the beep in milliseconds
void beep(int beepLength)
{
	digitalWrite(beeperPin, HIGH);
	delay(beepLength);
	digitalWrite(beeperPin, LOW);
}

// updateClock runs as part of the main loop reads the RTC unit and then updates 
// the display each time through the loop
void updateClock(){
	// grab the current time and date from the real time clock
	DateTime now = RTC.now();
	
	// write the clock interface to the LCD display
	char clockBuffer[16];
	sprintf(clockBuffer, "%02d:%02d:%02d %02d/%02d  ",now.hour(), now.minute(), now.second(),now.month(), now.day());
	drawDisplayTop("Cat Feeder 9000 ");
	drawDisplayBottom(String(clockBuffer));	
}

// checkFeedingTime runs as part of the main loop and will rotate both servos when the current time
// matches the hours and minutes config settings stored in the eeprom chip
void checkFeedingTime(){
	for (int i = 0; i < 2; i++)
	{
  	// grab the current time and date from the real time clock
  	DateTime now = RTC.now();

  	if ((config.feedingTimeHours[i] == now.hour()) && (config.feedingTimeMinutes[i] == now.minute()) && now.second() == 0)
  	{
      // Feeding time! Move left servo if it is feeding time #1; move right servo if #2
      beep(1000);
      if (i == 0){
      	moveServo(leftServoPin, leftLightPin);
      } 
      else {
      	moveServo(rightServoPin, rightLightPin);
      }
      
      
  }
}
}

// enterSettingsMenu allows the user to adjust the configuration settings that are stored in eeprom
// no matter where the user is in the menu, it will time out within 5 seconds if no input has been detected
// When the user presses the select button, the config variables are then stored in eeprom so that
// if power is lost, the unit will continue to function with the same settings
// If the user changes a setting and the menu times out, the information is not stored in eeprom
// The followings may be changed: Feeding Time 1, Feeding Time 2, Serving Size (deafult rotation time in milliseconds),
// and the Time Zone offset
void enterSettingsMenu(){
	String menuTop[7] = {"------Menu----->","<-----Menu----->","<-----Menu----->","<-----Menu----->","<-----Menu----->","<-----Menu----->","<-----Menu------"};
	String menuItem[7] = {"Set Feed Time 1 ", "Set Feed Time 2 ", "Set Serving Size", "Set Time Zone   ", "Set Time        " , "Set Month/Day   ", "Set Year        "};
	String settingsOverlay[7] = {"<[h]{      }[m]>", "<[h]{      }[m]>", "<[-]{      }[+]>", "<[-]{      }[+]>", "<[h]{      }[m]>", "<[m]{      }[d]>", "<[-]{      }[+]>"};
	int elapsedSeconds = 0;
	int startSeconds = millis();
	int currentMenuOption = 0;
	beep(10);
	drawDisplayTop(menuTop[currentMenuOption]);
	drawDisplayBottom(menuItem[currentMenuOption]);
	delay(50);

  // enter menu loop; menu loop times out upon five seconds
  while (elapsedSeconds < 5000){ 
  	elapsedSeconds = millis() - startSeconds;
  	uint8_t navButtons = lcd.readButtons();
  	if ((navButtons & BUTTON_LEFT) && (currentMenuOption > 0)) {
  		currentMenuOption--;
  		beep(10);
  		drawDisplayTop(menuTop[currentMenuOption]);
  		drawDisplayBottom(menuItem[currentMenuOption]);
  		delay(50);
  		startSeconds = millis();      
  	}
  	if ((navButtons & BUTTON_RIGHT) && (currentMenuOption < 6)) {
  		currentMenuOption++;
  		beep(10);
  		drawDisplayTop(menuTop[currentMenuOption]);
  		drawDisplayBottom(menuItem[currentMenuOption]);
  		delay(50);
  		startSeconds = millis();
  	}

    //display settings adjustment controls based off of location in menu
    if (navButtons & BUTTON_SELECT) { 
    	beep(10);
    	startSeconds = millis();
    	drawDisplayTop(menuItem[currentMenuOption]);
    	drawDisplayBottom(settingsOverlay[currentMenuOption]);
    	delay(50);
      //adjust settings based on location in menu
      switch (currentMenuOption) {
        case 0: // Feeding Time 1
        {
        	elapsedSeconds = adjustTime(config.feedingTimeHours[0], config.feedingTimeMinutes[0], "feedingTime1");
			break;
        }
        case 1: // Feeding Time 2
        {
		  	elapsedSeconds = adjustTime(config.feedingTimeHours[1], config.feedingTimeMinutes[1], "feedingTime2");
			break;
   		}  
        case 2: // Serving Size
        {
		  
        	elapsedSeconds = adjustRotationTime(config.defautRotationTime);
        	break;
   		} 
        case 3: // Time Zone
        {
        	elapsedSeconds = adjustTimeZone(config.offset);
        	break;
   		}
		case 4: // Time
		{
      		DateTime now = RTC.now(); //grab current time
      		elapsedSeconds = adjustTime(now.hour(), now.minute(), "clock");
			break;

       }
		case 5: // Month & Day
		{
			DateTime now = RTC.now(); //grab current month and day
			elapsedSeconds = adjustDate(now.month(), now.day());
			break;
		}
		case 6: // Year
		{
			DateTime now = RTC.now(); //grab current month and day
			elapsedSeconds = adjustYear(now.year());
			break;
		}
	}     
}  
}
lcd.clear();

}

// drawDisplayTop allows the user to input any string and outputs to the first line
// on the LCD display
void drawDisplayTop(String dispText){
	lcd.setCursor(0,0);
	lcd.print(dispText);
}

// drawDisplayBottom allows the user to input any string and outputs to the first line
// on the LCD display
void drawDisplayBottom(String dispText){
	lcd.setCursor(0,1);
	lcd.print(dispText);
}


// diaplayTimeZone formats the time zone integer with a plus sign or space to output
// in the proper location on the LCD display
// This was primarily designed for the settings menu and the time zone settings "overlay"
void displayTimeZone(int timeZone){
	lcd.setCursor(7,1);
	char timeZoneBuffer[5];
  if (timeZone == 0){ //show the space on zero or plus sign on positive numbers
  	sprintf(timeZoneBuffer, " %01d", timeZone);   
  	}else if (timeZone > 0){
  		sprintf(timeZoneBuffer, "+%01d ", timeZone);
  		}else {
  			sprintf(timeZoneBuffer, "%01d ", timeZone);
  		}
  		lcd.print(String(timeZoneBuffer));
  	}


// displayRotationTime positions the rotation time integer in the proper location on the
// LCD and adds the Symbol for milliseconds to indicate as such
void displayRotationTime(int time){
	lcd.setCursor(5,1);
	lcd.print(String(time) + "ms");
}

// displayTimeDigits takes the feeding hour or minutes integer then outputs
// it in the specified loation on the LCD. In addition, it adds aleading zero to the
// so that it appears in a consistant format
void displayTimeDigits(int feedingDigits, int horizontalLocation){
	lcd.setCursor(horizontalLocation,1);
  if (feedingDigits < 10) { // add leading zero
  	lcd.print("0" + String(feedingDigits));
  	} else {
  		lcd.print(feedingDigits);
  	}
  }

// displayTimeColon outputs a colon to the display for the feeding time settings menu
void displayTimeColon(){
	lcd.setCursor(7,1);
	lcd.print(": ");
}

// displayDateMenu prints the date inside of the menu overlay on the LCD
void displayDateMenu(int displayMonth, int displayDay){
	lcd.setCursor(5, 1);
	char dateBuffer[6];
	sprintf(dateBuffer, "%02d/ %02d", displayMonth, displayDay);
	lcd.print(String(dateBuffer));
}

// displayYearMenu prints the Year inside of the menu overlay on the LCD
void displayYearMenu(int displayYear) {
	lcd.setCursor(6, 1);
	char yearBuffer[6];
	sprintf(yearBuffer, "%04d ", displayYear);
	lcd.print(String(yearBuffer));
}

// adjustTime Allows the changing of the time in the menu; you may choose either "clock" or
//	"feedingTime1"/"feedingTime2" in the adjustment type. This determins whether one needs to set the RTC or save 
//	feeding times to eeprom. An integer set to 5000 is returned to break out of the 
//  5 second timer and return to the home screen
int adjustTime(int adjustedHour, int adjustedMinute, String adjustmentType){
	int elapsedSeconds = 0;
	int startSeconds = millis();
	displayTimeDigits(adjustedHour, 5);
    displayTimeColon();
    displayTimeDigits(adjustedMinute, 9);
    while (elapsedSeconds < 5000) {
    	elapsedSeconds = millis() - startSeconds;
		// check navigation buttons
		uint8_t navButtons = lcd.readButtons();
		if (navButtons & BUTTON_LEFT){ //increment hours
			beep(10);
			if (adjustedHour == 23){
				adjustedHour = 0;
				} else {
					adjustedHour++;
				}
				displayTimeDigits(adjustedHour, 5);
				delay(250);
				startSeconds = millis();
			}

			if (navButtons & BUTTON_RIGHT){ //increment minutes
				beep(10);
				if (adjustedMinute == 59){
					adjustedMinute = 0;
					} else {
						adjustedMinute++;
					}
					displayTimeDigits(adjustedMinute, 9);
					delay(150);
					startSeconds = millis();
				}

				if (navButtons & BUTTON_SELECT){
					beep(10);
					delay(250);
					if (adjustmentType == "clock") {
						//adjust RTC hours and minutes
						DateTime now = RTC.now();
						RTC.adjust(DateTime(now.year(), now.month(), now.day(), adjustedHour, adjustedMinute, 0));
					} else if (adjustmentType == "feedingTime1") {
						// save vals to eeprom
						config.feedingTimeHours[0] = adjustedHour;// save temp vars to config
				   		config.feedingTimeMinutes[0] = adjustedMinute;
				   		eeWrite(0, config); // write new config to eeprom
					} else if (adjustmentType == "feedingTime2") {
						// save vals to eeprom
						config.feedingTimeHours[1] = adjustedHour;// save temp vars to config
				   		config.feedingTimeMinutes[1] = adjustedMinute;
				   		eeWrite(0, config); // write new config to eeprom
					} else {
						// nothing recognizable was passed as the adjustment type; display error message
						drawDisplayTop("ERR BAD PARAMS  ");
						drawDisplayBottom("                ");
						drawDisplayBottom(adjustmentType);
						beep(500);
						delay(500);
						beep(500);
						delay(500);
						beep(500);
					}
					delay(50);
	               	return 5000;// exit menu to home screen
	            }
	}
}

// adjustYear allows for setting the year in the RTC module
// An integer set to 5000 is returned to break out of the 5 second timer and return to the home screen
int adjustYear(int adjustedYear) {
	int elapsedSeconds = 0;
	int startSeconds = millis();
	displayYearMenu(adjustedYear);
	while (elapsedSeconds < 5000) {
    	elapsedSeconds = millis() - startSeconds;
		// check navigation buttons
		uint8_t navButtons = lcd.readButtons();
		if ((navButtons & BUTTON_LEFT) && (adjustedYear > 2000)){ //increment Year
			beep(10);
			adjustedYear -= 1;
			displayYearMenu(adjustedYear);
			delay(250);
			startSeconds = millis();
		}
		if ((navButtons & BUTTON_RIGHT) && (adjustedYear < 2100)){ //decrement Year
			beep(10);
			adjustedYear += 1;
			displayYearMenu(adjustedYear);
			delay(250);
			startSeconds = millis();
		}
		if (navButtons & BUTTON_SELECT){
	       	beep(10);
	       	DateTime now = RTC.now();
	       	delay(250);
	       	RTC.adjust(DateTime(adjustedYear, now.month(), now.day(), now.hour(), now.minute(), now.second()));
		    delay(50);
	        return 5000;//exit menu to home screen    
	    }
	}
}

// adjustDate allows for setting the month and day in the RTC
// An integer set to 5000 is returned to break out of the 5 second timer and return to the home screen
int adjustDate(int adjustedMonth, int adjustedDay) {
	int elapsedSeconds = 0;
	int startSeconds = millis();
	displayDateMenu(adjustedMonth, adjustedDay);
	while (elapsedSeconds < 5000) {
    	elapsedSeconds = millis() - startSeconds;
		// check navigation buttons
		uint8_t navButtons = lcd.readButtons();
		if (navButtons & BUTTON_LEFT){ //increment Months
			beep(10);
			adjustedMonth += 1;
			if (adjustedMonth == 13) {
				adjustedMonth = 1;
			}
			displayDateMenu(adjustedMonth, adjustedDay);
			delay(250);
			startSeconds = millis();
		}
		if (navButtons & BUTTON_RIGHT){ //increment Days
			beep(10);
			adjustedDay += 1;
			if (adjustedDay == 32) {
				adjustedDay = 1;
			} 
			displayDateMenu(adjustedMonth, adjustedDay);
			delay(250);
			startSeconds = millis();
		}
		if (navButtons & BUTTON_SELECT){
	       	beep(10);
	       	DateTime now = RTC.now();
	       	delay(250);
	       	RTC.adjust(DateTime(now.year(), adjustedMonth, adjustedDay, now.hour(), now.minute(), now.second()));
		    delay(50);
	        return 5000;//exit menu to home screen    
	    }
	}
}

// adjustRotationTime allows for setting the rotation time (or serving size of cat food) in the menu
// An integer set to 5000 is returned to break out of the 5 second timer and return to the home screen
int adjustRotationTime (int adjustedRotationTime) {
	int elapsedSeconds = 0;
	int startSeconds = millis();
	displayRotationTime(adjustedRotationTime);
  	while (elapsedSeconds < 5000){
  		elapsedSeconds = millis() - startSeconds;
    	// check navigation buttons
    	uint8_t navButtons = lcd.readButtons();
    	if ((navButtons & BUTTON_LEFT) && (adjustedRotationTime > 200)){
    		beep(10);
    		adjustedRotationTime -= 10;
    		displayRotationTime(adjustedRotationTime);
    		delay(50);
    		startSeconds = millis();
    	}
    
    	if ((navButtons & BUTTON_RIGHT) && (adjustedRotationTime < 990)){
    		beep(10);
    		adjustedRotationTime += 10;
    		displayRotationTime(adjustedRotationTime);
    		delay(50);
    		startSeconds = millis();
    	}
    
    	if (navButtons & BUTTON_SELECT){
    		beep(10);
    		delay(250);
	   		config.defautRotationTime = adjustedRotationTime; // save temp var to config
	   		eeWrite(0, config); // write new config to eeprom
	   		delay(50);
       		return 5000;//exit menu to home screen    
   		}
	}
}

// adjustTimeZone allows for setting the time zone setting in the menu
// An integer set to 5000 is returned to break out of the 5 second timer and return to the home screen
int adjustTimeZone(int adjustedTimeZone) {
	int elapsedSeconds = 0;
	int startSeconds = millis();
	displayTimeZone(adjustedTimeZone);
	while (elapsedSeconds < 5000){
		elapsedSeconds = millis() - startSeconds;
	    // check navigation buttons
	    uint8_t navButtons = lcd.readButtons();
	    if ((navButtons & BUTTON_LEFT) && (adjustedTimeZone > -11)){
	       	beep(10);
	       	adjustedTimeZone--;
	       	displayTimeZone(adjustedTimeZone);
	       	delay(250);
	       	startSeconds = millis();
	    }
	    if ((navButtons & BUTTON_RIGHT) && (adjustedTimeZone < 14)){
	      	beep(10);
	       	adjustedTimeZone++;
	       	displayTimeZone(adjustedTimeZone);
	       	delay(250);
	       	startSeconds = millis();
	    }
	    if (navButtons & BUTTON_SELECT){
	       	beep(10);
	       	delay(250);
	        // update real time clock (hn - ofn) + ofc = hs
	        DateTime now = RTC.now();
	        int hourSet = (now.hour() - config.offset) + adjustedTimeZone;
	        if (hourSet > 23) {
	          	hourSet -= 24;
	       	} else if (hourSet < 0) {
	       		hourSet += 24;
	       	}
	       	RTC.adjust(DateTime(now.year(), now.month(), now.day(), hourSet, now.minute(), now.second()));
		    config.offset = adjustedTimeZone;// save temp var to config
		    eeWrite(0, config); // write new config to eeprom
		    delay(50);
	        return 5000;//exit menu to home screen    
	    }
    }
}