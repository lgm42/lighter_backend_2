#include <ArduinoJson.h>
#include <FS.h>

#include "ParameterProvider.h"

const String ParameterProvider::Filename = "/config.json";

ParameterProvider::ParameterProvider()
{}

ParameterProvider::~ParameterProvider()
{}

void ParameterProvider::setup()
{
  SPIFFS.begin(); 
}

void ParameterProvider::load()
{
  // Open file for reading
  File file = SPIFFS.open(Filename, "r");

  DynamicJsonDocument doc(1024);

  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    createDefaultValues();
    return;
  }

  _currentParameters.hostname = doc["hostname"] | "lighter";
	_currentParameters.ftpLogin = doc["ftp.login"] | "lighter";
	_currentParameters.ftpPasswd = doc["ftp.passwd"] | "lighter";
	_currentParameters.automaticMode = doc["automaticMode"] | true;

	_currentParameters.timeStart = doc["timeStart"] | (8 * 3600);
	_currentParameters.timeEnd = doc["timeEnd"] | (18 * 3600);
	_currentParameters.startDuration = doc["startDuration"] | (15 * 60);
	_currentParameters.endDuration = doc["endDuration"] | (15 * 60);

  Serial.printf("hostname : %s\n", _currentParameters.hostname.c_str());
	Serial.printf("ftpLogin : %s\n", _currentParameters.ftpLogin.c_str());
	Serial.printf("ftpPasswd : %s\n", _currentParameters.ftpPasswd.c_str());
	Serial.printf("automaticMode : %d\n", _currentParameters.automaticMode);

  // Close the file (File's destructor doesn't close the file)
  file.close();
}

void ParameterProvider::save()
{
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(Filename);

  // Open file for writing
  File file = SPIFFS.open(Filename, "w");
  if (!file) {
      Serial.println(F("Failed to create file"));
      return;
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, file);

  doc["hostname"] = _currentParameters.hostname;
	doc["ftp.login"] = _currentParameters.ftpLogin;
	doc["ftp.passwd"] = _currentParameters.ftpPasswd;
	doc["automaticMode"] = _currentParameters.automaticMode;
	
	doc["timeStart"]		 = _currentParameters.timeStart 		;				 
	doc["timeEnd"]			 = _currentParameters.timeEnd 		;				 
	doc["startDuration"]	 = 	_currentParameters.startDuration ;				 
	doc["endDuration"]		 = _currentParameters.endDuration 	;				 

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
      Serial.println(F("Failed to write to file"));
  }

  // Close the file (File's destructor doesn't close the file)
  file.close();
}

void ParameterProvider::createDefaultValues()
{
  _currentParameters.hostname = "lighter";
	_currentParameters.ftpLogin = "lighter";
	_currentParameters.ftpPasswd = "lighter";
	_currentParameters.automaticMode = true;

	_currentParameters.timeStart = 8 * 3600; //8h00
	_currentParameters.timeEnd = 18 * 3600; //18h00
	_currentParameters.startDuration = 15 * 60; //15 min
	_currentParameters.endDuration = 15 * 60; //15 min
}

ParameterProvider::Parameters & ParameterProvider::params()
{
  return _currentParameters;
}

const ParameterProvider::Parameters & ParameterProvider::params() const 
{
  return _currentParameters;
}

String ParameterProvider::toJson() const
{
	String response = "{\"hostname\":\"" + _currentParameters.hostname + "\", 					\
	\"ftp-login\":\"" + _currentParameters.ftpLogin + "\",					\
	\"ftp-passwd\":\"" + _currentParameters.ftpPasswd + "\",					\
	\"time-start\":\"" + _currentParameters.timeStart + "\",					\
	\"time-end\":\"" + _currentParameters.timeEnd + "\",					\
	\"start-duration\":\"" + _currentParameters.startDuration + "\",					\
	\"end-duration\":\"" + _currentParameters.endDuration + "\"}";
	return response;
}
