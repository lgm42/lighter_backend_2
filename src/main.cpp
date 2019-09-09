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

ParameterProvider _paramsProvider;
StatusHandler _statusHandler(_paramsProvider);
HttpServer _httpServer(_paramsProvider, _statusHandler);
Ticker _ticker;

void tick()
{
	//every second we check time only in automatic mode
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
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();

  delay(500);
	pinMode(Constants::LedPinNumber, OUTPUT);  
  //TODO : add to system parameters
  wifiManager.autoConnect("Ligter_AP");
  
  //if you get here you have connected to the WiFi
  Serial.println("connected...");

  ArduinoOTA.setHostname("wemos_lighter"); 
  ArduinoOTA.begin();

  _paramsProvider.setup();
  _statusHandler.setup();
  _httpServer.setup();

  analogWrite(Constants::LedPinNumber, 255);

  //install timer
	_ticker.attach(1, tick);
}

void loop() {
  
  Serial.println("loop...");
  ArduinoOTA.handle();
  _httpServer.handle();
  delay(100);
}

