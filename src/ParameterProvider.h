#pragma once

#include <Arduino.h>

class ParameterProvider
{
  public:

    typedef struct
    {
        String hostname;
        String ftpLogin;
        String ftpPasswd;

        int timeStart;
        int timeEnd;
        int startDuration;
        int endDuration;
        
        bool automaticMode;
    } Parameters;

    ParameterProvider();
    virtual ~ParameterProvider();

    void setup();

    void load();
    void save();

    void createDefaultValues();
    Parameters & params();
    const Parameters & params() const;
    String toJson() const;

  private:
    Parameters _currentParameters;
    static const String Filename;
};