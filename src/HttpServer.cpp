// 
// 
// 
#include <time.h>
#include <FS.h>

#include "ParameterProvider.h"
#include "StatusHandler.h"

#include "HttpServer.h"

// Timezone
#define UTC_OFFSET + 1
#define DAYLIGHT_OFFSET 3600

// change for different ntp (time servers)
#define NTP_SERVERS "0.fr.pool.ntp.org", "time.nist.gov", "pool.ntp.org"

const String HttpServer::HostnameKeyword = "hostname";
const String HttpServer::FtpLoginKeyword = "ftp-login";
const String HttpServer::FtpPasswdKeyword = "ftp-passwd";
const String HttpServer::TimeStartKeyword = "time-start";
const String HttpServer::TimeEndKeyword = "time-end";
const String HttpServer::StartDurationKeyword = "start-duration";
const String HttpServer::EndDurationKeyword = "end-duration";
const String HttpServer::ValueWhenLowKeyword = "value-low";
const String HttpServer::ValueWhenHighKeyword = "value-high";

String padWithZero(String input)
{
	if (input.length() == 1)
		return "0" + input;
	else
		return input;
}

String formatTime()
{
	time_t now;
	time(&now);
	struct tm * timeinfo;
	timeinfo = localtime(&now);
	return padWithZero(String(timeinfo->tm_hour)) + ":" + padWithZero(String(timeinfo->tm_min)) + ":" + padWithZero(String(timeinfo->tm_sec));
}

int HttpServer::localTime() const
{
	time_t now;
	time(&now);
	struct tm * timeinfo;
	timeinfo = localtime(&now);
	return timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;
}

HttpServer::HttpServer(ParameterProvider & params, StatusHandler & statusHandler)
	: _webServer(80), _paramProvider(params), _httpUpdater(true), _statusHandler(statusHandler)
{
}

HttpServer::~HttpServer()
{
}


void HttpServer::setup(void)
{
	_paramProvider.load();
	
	MDNS.begin(_paramProvider.params().hostname.c_str()); 
	MDNS.addService("http", "tcp", 80);

	_ftpServer.begin(_paramProvider.params().ftpLogin, _paramProvider.params().ftpPasswd);
	MDNS.addService("ftp", "tcp", 21);

    _httpUpdater.setup(&_webServer, "/update");
	_webServer.begin();

	updateNTP();

    _webServer.on("/rest/reboot", [&]() {
        _webServer.send(200, "text/plain", "ESP reboot now !");
        delay(200);
        ESP.restart();
    });
	
    _webServer.on("/rest/led", [&]() {
        Serial.println("REST: " + _webServer.uri());
		if (_statusHandler.mode() != StatusHandler::kAutomatic)
		{
			String value = _webServer.arg("value");
			int val = value.toInt();
			_statusHandler.currentValue(val);
			sendOk();
		}
		else
		{
			sendKo("Unable to drive led in automatic mode");
		}
    });

    _webServer.on("/rest/status", [&]() {
		Serial.println("REST: " + _webServer.uri());
		Serial.println("sending status");
		String result = "{\"mode\":\"" + String((int)_statusHandler.mode()) + 
					 "\", \"currentValue\" : \"" + String(_statusHandler.currentValue()) + 
					 "\", \"time\" : \"" + formatTime() +
					 "\", \"params\" : " + _paramProvider.toJson() + 
					"}";

		sendOkAnswerWithParams(result);
    });

    _webServer.on("/rest/mode", [&]() {
		Serial.println("REST: " + _webServer.uri());
		String value = _webServer.arg("value");
		if (value == "auto")
		{
			Serial.println("going to automatic mode");
			_statusHandler.mode(StatusHandler::kAutomatic);
			sendOk();
		}
		else if (value == "manual")
		{
			Serial.println("going to manual mode");
			_statusHandler.mode(StatusHandler::kManual);
			sendOk();
		}
		else if (value == "forcedManual")
		{
			Serial.println("going to forced manual mode");
			_statusHandler.mode(StatusHandler::kForcedManual);
			sendOk();
		}
		else
			sendKo("unknown mode");
    });
	
    _webServer.on("/rest/params", [&]() {
		Serial.println("REST: " + _webServer.uri());
		sendOkAnswerWithParams(_paramProvider.toJson());
    });
	
    _webServer.on("/rest/param", [&]() {
		Serial.println("REST: " + _webServer.uri());
		String name = _webServer.arg("name");
		String value = _webServer.arg("value");

		if (name == HostnameKeyword)
		{
			if (value != "")
			{
				_paramProvider.params().hostname = value;
				_paramProvider.save();
			}
			sendOkAnswerWithParams("\"" + (String)_paramProvider.params().hostname + "\"");
		} 
		else if (name == FtpLoginKeyword)
		{
			if (value != "")
			{
				_paramProvider.params().ftpLogin = value;
				_paramProvider.save();
			}
			sendOkAnswerWithParams("\"" + (String)_paramProvider.params().ftpLogin + "\"");
		} 
		else if (name == FtpPasswdKeyword)
		{
			if (value != "")
			{
				_paramProvider.params().ftpPasswd = value;
				_paramProvider.save();
			}
			sendOkAnswerWithParams("\"" + (String)_paramProvider.params().ftpPasswd + "\"");
		} 
		else if (name == TimeStartKeyword)
		{
			if (value != "")
			{
				_paramProvider.params().timeStart = value.toInt();
				_paramProvider.save();
			}
			sendOkAnswerWithParams("\"" + (String)_paramProvider.params().timeStart + "\"");
		} 
		else if (name == TimeEndKeyword)
		{
			if (value != "")
			{
				_paramProvider.params().timeEnd = value.toInt();
				_paramProvider.save();
			}
			sendOkAnswerWithParams("\"" + (String)_paramProvider.params().timeEnd + "\"");
		} 
		else if (name == StartDurationKeyword)
		{
			if (value != "")
			{
				_paramProvider.params().startDuration = value.toInt();
				_paramProvider.save();
			}
			sendOkAnswerWithParams("\"" + (String)_paramProvider.params().startDuration + "\"");
		} 
		else if (name == EndDurationKeyword)
		{
			if (value != "")
			{
				_paramProvider.params().endDuration = value.toInt();
				_paramProvider.save();
			}
			sendOkAnswerWithParams("\"" + (String)_paramProvider.params().endDuration + "\"");
		} 
		else
		{
			sendKo("unknown param name");
		}
    });

	//called when the url is not defined here
	//use it to load content from SPIFFS
	_webServer.onNotFound([&]() {
		if (!handleFileRead(_webServer.uri()))
			_webServer.send(404, "text/plain", "FileNotFound");
	});
}

String HttpServer::getContentType(String filename)
{
	if (_webServer.hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

bool HttpServer::handleFileRead(String path)
{
	_webServer.sendHeader("Access-Control-Allow-Origin", "*");
	
    if (path.endsWith("/")) 
        path += "index.html";

    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
        if (SPIFFS.exists(pathWithGz))
            path += ".gz";
        File file = SPIFFS.open(path, "r");
        _webServer.streamFile(file, contentType);
        file.close();
        return true;
    }

	return false;
}

void HttpServer::sendOk()
{
	_webServer.send(200, "application/json", "{\"result\":true}");
}

void HttpServer::sendOkAnswerWithParams(const String & params)
{
	String data("{\"result\":true, \"data\":");
	data += params;
	data += "}";
	_webServer.send(200, "application/json", data);
}

void HttpServer::sendKo(const String & message)
{
	String data("{\"result\":false, \"message\":\"");
	data += message;
	data += "\"}";
	_webServer.send(400, "application/json", data);
}

void HttpServer::updateNTP() 
{
  configTime(UTC_OFFSET * 3600, DAYLIGHT_OFFSET, NTP_SERVERS);
  delay(500);
  while (!time(nullptr)) {
    Serial.print("#");
    delay(1000);
  }
  Serial.println("Update NTP");
}

void HttpServer::handle(void)
{
	_webServer.handleClient();
	_ftpServer.handleFTP();
    MDNS.update();
}

ESP8266WebServer & HttpServer::webServer() 
{
	return _webServer;
}