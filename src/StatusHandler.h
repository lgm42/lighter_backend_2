#pragma once

class StatusHandler 
{
public:
	typedef enum  
	{
		kAutomatic = 0,
		kManual = 1,
		kForcedManual = 2
	} LightMode;

	StatusHandler(ParameterProvider & paramsProvider);
	virtual ~StatusHandler();

	virtual void setup();

	String toJson();

	void currentValue(const int value);
	int currentValue() const;

	void mode(const StatusHandler::LightMode mode);
	StatusHandler::LightMode mode() const;

	void forceManual(const int time);
	int remainingTimeInForcedMode() const;
	void decreaseRemainingTimeInForcedMode();
	
private:
	StatusHandler::LightMode _mode;
	int _currentValue;
	ParameterProvider & _paramsProvider;
	int _remainingTimeInForcedMode;
};
