// HttpServer.h

#pragma once

#include "Arduino.h"

#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ESP8266FtpServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include "StatusHandler.h"
#include "ParameterProvider.h"

class HttpServer
{
  public:
	  HttpServer(ParameterProvider & params, StatusHandler & statusHandler);
	  virtual ~HttpServer();

	virtual void setup(void);
	virtual void handle(void);

	String getContentType(String filename);

	ESP8266WebServer & webServer();
	int localTime() const;

private:
	ESP8266WebServer _webServer;
	FtpServer _ftpServer;
	WiFiUDP _ntpUDP;
	ParameterProvider & _paramProvider;
  ESP8266HTTPUpdateServer _httpUpdater;
	StatusHandler & _statusHandler;

	void updateNTP();
	bool handleFileRead(String path);

	void sendOk();
	void sendKo(const String & message);
	void sendOkAnswerWithParams(const String & params);

	static const String HostnameKeyword;
	static const String FtpLoginKeyword;
	static const String FtpPasswdKeyword;
	static const String TimeOffsetKeyword;

	static const String TimeStartKeyword;
	static const String TimeEndKeyword;
	static const String StartDurationKeyword;
	static const String EndDurationKeyword;
	static const String ValueWhenLowKeyword;
	static const String ValueWhenHighKeyword;
};