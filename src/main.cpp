#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <Ticker.h>

#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include "ParameterProvider.h"
#include "HttpServer.h"
#include "StatusHandler.h"
#include "Constants.h"

void ICACHE_RAM_ATTR handleButtonPressInterrupt();

ParameterProvider _paramsProvider;
StatusHandler _statusHandler(_paramsProvider);
HttpServer _httpServer(_paramsProvider, _statusHandler);
Ticker _ticker;
WiFiManager _wifiManager;

void tick()
{
	//every second we check time only in automatic mode
  Serial.println("mode " + String((int)_statusHandler.mode()));
	if (_statusHandler.mode() == StatusHandler::kAutomatic)
	{
		int time = _httpServer.localTime();
		//we get time from midnight
		time = time % 86400;
		/*Serial.println("time");
		Serial.println(time);*/
		
		int lowerBoundTime = 0;
		int higherBoundTime = 1;
		int lowerBoundValue = 0;
		int higherBoundValue = 1;

		int newValue = -1;

		if (time < _paramsProvider.params().timeStart)
		{
			//off before light
			newValue = 0;
		}
		else if (time < _paramsProvider.params().timeStart + _paramsProvider.params().startDuration)
		{
			//sunrise
			lowerBoundTime = _paramsProvider.params().timeStart;
			lowerBoundValue = 0;
			higherBoundTime = _paramsProvider.params().timeStart + _paramsProvider.params().startDuration;
			higherBoundValue = 100;
		}
		else if (time < _paramsProvider.params().timeEnd)
		{
			//lighted
			newValue = 100;
		}
		else if (time < _paramsProvider.params().timeEnd + _paramsProvider.params().endDuration)
		{
			//sunset
			lowerBoundTime = _paramsProvider.params().timeEnd;
			lowerBoundValue = 100;
			higherBoundTime = _paramsProvider.params().timeEnd + _paramsProvider.params().endDuration;
			higherBoundValue = 0;
		}
		else
		{
			//off after light
			newValue = 0;
		}

		if (newValue == -1)
		{
			//newValue hasn't been set 
			float implication = ((float)time - lowerBoundTime) / (higherBoundTime - lowerBoundTime);
			newValue = (int)(implication * higherBoundValue + (1.0 - implication) * lowerBoundValue);
		}

		/*Serial.println("new Value");
		Serial.println(newValue);*/
		_statusHandler.currentValue(newValue);
	}
  else if (_statusHandler.mode() == StatusHandler::kForcedManual)
  {
    _statusHandler.decreaseRemainingTimeInForcedMode();
    if (_statusHandler.remainingTimeInForcedMode() < 0)
    {
      //finished !
      _statusHandler.mode(StatusHandler::kAutomatic);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //_WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around

  //reset saved settings
  //_wifiManager.resetSettings();

  delay(500);
  pinMode(Constants::LedPinNumber, OUTPUT);  
  pinMode(Constants::ButtonPinNumber, INPUT);
  //attachInterrupt(digitalPinToInterrupt(Constants::ButtonPinNumber), handleButtonPressInterrupt, FALLING);

  //TODO : add to system parameters
  _wifiManager.autoConnect("Ligter_AP");
  
  //if you get here you have connected to the WiFi
  Serial.println("connected...");

  ArduinoOTA.setHostname("wemos_lighter"); 
  ArduinoOTA.begin();

  _paramsProvider.setup();
  _paramsProvider.load();
  _statusHandler.setup();
  _httpServer.setup();

  analogWrite(Constants::LedPinNumber, 255);

  //install timer
	_ticker.attach(1, tick);
}

void loop() {
  
  //Serial.println("loop...");
  ArduinoOTA.handle();
  _httpServer.handle();
  if (digitalRead(Constants::ButtonPinNumber) == 0)
  {
    Serial.println("Button pressed");
    if (_statusHandler.mode() != StatusHandler::kManual)
    {
      _statusHandler.forceManual(Constants::ManualModeTimeWhenButtonPressed);
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
	  _wifiManager.autoConnect("Ligter_AP");
  }
  delay(10);
}
/*
void ICACHE_RAM_ATTR handleButtonPressInterrupt() 
{
  //on pressed
  if (_statusHandler.mode() != StatusHandler::kManual)
    _statusHandler.forceManual(Constants::ManualModeTimeWhenButtonPressed);
}*/
