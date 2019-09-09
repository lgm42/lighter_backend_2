// 
// 
#include "Constants.h"
#include "ParameterProvider.h"
#include "StatusHandler.h"

StatusHandler::StatusHandler(ParameterProvider & paramsProvider)
	: _paramsProvider(paramsProvider)
{
}

StatusHandler::~StatusHandler()
{
}

void StatusHandler::setup(void)
{
	if (_paramsProvider.params().automaticMode)
		_mode = kAutomatic;
	else
		_mode = kManual;

	//we force update
	_currentValue = -1;
	currentValue(0);
}

void StatusHandler::mode(const StatusHandler::LightMode mode)
{
	if (mode != _mode)
	{
		_mode = mode;
		if (mode == kAutomatic)
			_paramsProvider.params().automaticMode = true;
		else
			_paramsProvider.params().automaticMode = false;
		_paramsProvider.save();
	}
}

StatusHandler::LightMode StatusHandler::mode() const
{
	return _mode;
}

void StatusHandler::currentValue(const int value)
{
	//Serial.printf("Setting led to %d\n", value);
	_currentValue = value;
	
	//we apply LUT (cf Excel file)
	//0,0005*PUISSANCE(E1;3)+0,0585*E1*E1-0,626*E1
	int lutValue = (int)(0.0005 * value * value * value + 0.0585 * value * value - 0.626 * value);
	lutValue = max(min(1023, lutValue), 0);
	analogWrite(Constants::LedPinNumber, lutValue);
}

int StatusHandler::currentValue() const
{
	return _currentValue;
}
