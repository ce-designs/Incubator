
#include <Pushbutton.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// PINS
#define ONE_WIRE_BUS 10
#define HEATER_RELAY_PIN 8
#define BTN_LEFT_PIN 2
#define BTN_RIGHT_PIN 3

// Buttons
 
Pushbutton btnLeft(BTN_LEFT_PIN);
Pushbutton btnRight(BTN_RIGHT_PIN);

// DS18B20 Temperature probe
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// LCD 
LiquidCrystal_I2C lcd(0x20);

// EEPROM
int addrInitialized = 0;			// EEPROM address for storing the initialized parameter
int addrSetTemp = 1;				// EEPROM address for storing the setTemp value	
const byte eepromInitialized = 101;	// constant for determining if the the EEPROM has been initialized

// settings
const unsigned long readInterval = 5000;
const unsigned long OnOffInterval = 30000;
const unsigned long saveSettingsDelay = 2000;
const float tempDifference = 2;
const float tempThreshold = 0.3;

// variables for keeping the incubator status
bool heaterIsOn = false;
float setTemp = 22;
float currentTemp;
float prevTemp;
bool overShootControlActive;

// variables for storing the time of an associated action
unsigned long readTempMillis;
unsigned long heaterOnMillis;
unsigned long heaterOffMillis;
unsigned long printTimeMillis;

void setup()
{
	Serial.begin(9600);

	// Initialize the EEPROM if not initialized
	if (EEPROM.read(addrInitialized) != eepromInitialized)
	{
		EEPROM.update(addrInitialized, eepromInitialized);
		SaveSettings();
	}
	// Read from the EEPROM
	setTemp = EEPROM.read(addrSetTemp);	

	pinMode(HEATER_RELAY_PIN, OUTPUT);
	
	// Start up the DallasTemperature library 
	sensors.begin();

	lcd.begin(20, 4);	

	sensors.requestTemperatures();
	currentTemp = sensors.getTempCByIndex(0);

	PrintMainMenu();
}

void loop()
{	
	if (btnLeft.getSingleDebouncedRelease())
	{
		setTemp--;
		UpdateMainMenuSetTemp();
		delay(200);
	}
	else if (btnRight.getSingleDebouncedRelease())
	{
		setTemp++;
		UpdateMainMenuSetTemp();
		delay(200);
	}
	else if (btnLeft.isPressed() && btnRight.isPressed())
	{
		long t = millis();
		while (btnLeft.isPressed() && btnRight.isPressed())
		{
			if (millis() - t > saveSettingsDelay)
			{
				SaveSettings();
				PrintSavedSettings();
				delay(2500);
				PrintMainMenu();
				break;
			}
		}
	}

	if (readTemperatureWithInterval() && prevTemp != currentTemp)
	{
		UpdateMainMenuCurrentTemp();
		prevTemp = currentTemp;
	}

	overShootControlActive = (setTemp - currentTemp <= tempDifference);

	if (overShootControlActive)
	{
		if (currentTemp + tempThreshold < setTemp && !heaterIsOn && (millis() - heaterOffMillis > OnOffInterval))
		{
			turnHeaterOn();
			UpdateMainMenuHeaterState();
		}
		else if (heaterIsOn && (currentTemp >= setTemp || ((millis() - heaterOnMillis > OnOffInterval))))
		{
			turnHeaterOff();
			UpdateMainMenuHeaterState();
		}
	}
	else if (currentTemp < setTemp && !heaterIsOn)
	{		
		turnHeaterOn();
		UpdateMainMenuHeaterState();
	}	

	PrintTimeWithInterval();
}

/// <summary>
/// Saves the minTemp, maxTemp and fanSpeed settings to the EEPROM
/// </summary>
void SaveSettings()
{
	EEPROM.update(addrSetTemp, setTemp);
}

void PrintTimeWithInterval()
{
	long currentTime = now();
	if (currentTime - printTimeMillis < 1)
		return;	
	int days = elapsedDays(currentTime);
	int hours = numberOfHours(currentTime);
	int minutes = numberOfMinutes(currentTime);
	int seconds = numberOfSeconds(currentTime);

	// digital clock display of current time
	lcd.setCursor(0, 1);
	lcd.print(days, DEC);
	printDigits(hours);
	printDigits(minutes);
	printDigits(seconds);
	printTimeMillis = currentTime;
}

void printDigits(byte digits)
{
	// utility function for digital clock display: prints colon and leading 0
	lcd.print(":");
	if (digits < 10)
		lcd.print('0');
	lcd.print(digits, DEC);
}

void PrintSavedSettings()
{
	lcd.clear();
	lcd.setCursor(5, 1);
	lcd.print("SETTINGS");	
	lcd.setCursor(7, 2);
	lcd.print("SAVED");
}

void PrintMainMenu()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Current Temp:");
	lcd.setCursor(0, 2);
	lcd.print("Heater:");
	lcd.setCursor(0, 3);
	lcd.print("Set Temp:");
	UpdateMainMenuSetTemp();
	UpdateMainMenuCurrentTemp();
	UpdateMainMenuHeaterState();
}

/// <summary>
/// Updates the current temperature in the main menu
/// </summary>
void UpdateMainMenuCurrentTemp()
{
	lcd.setCursor(14, 0);
	if (currentTemp < 10)
		lcd.print(" ");
	lcd.print(currentTemp);
	lcd.print("C");
}

/// <summary>
/// Updates the heater state in the main menu
/// </summary>
void UpdateMainMenuHeaterState()
{
	lcd.setCursor(8, 2);
	if (heaterIsOn)
		lcd.print("ON ");
	else
		lcd.print("OFF");	
}

/// <summary>
/// Updates set temperature in the main menu
/// </summary>
void UpdateMainMenuSetTemp()
{
	lcd.setCursor(10, 3);
	if (setTemp < 10)
		lcd.print(" ");
	lcd.print(setTemp);
	lcd.print("C");
}

bool readTemperatureWithInterval()
{
	if (millis() - readTempMillis < readInterval)
		return false;		
	sensors.requestTemperatures();
	currentTemp = sensors.getTempCByIndex(0);
	readTempMillis = millis();
	return true;
}

void turnHeaterOn()
{
	heaterIsOn = true;
	digitalWrite(HEATER_RELAY_PIN, HIGH);
	heaterOnMillis = millis();
}

void turnHeaterOff()
{
	heaterIsOn = false;
	digitalWrite(HEATER_RELAY_PIN, LOW);
	heaterOffMillis = millis();
}

