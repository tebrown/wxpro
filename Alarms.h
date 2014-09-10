#ifndef __ALARMS_H__
#define __ALARMS_H__
#include "WeatherSink.h"
#include <map>

#define ALARM_INTERVAL (10)

class Alarms: public WeatherSink
{
public:
   Alarms( Config cfg, WeatherData *wd );
   ~Alarms();
   
   void newData();
   void setLimits();
   void main();

private:
   pthread_mutex_t notify;
   std::map< WeatherData::PROPERTY, double > lowerLimit;
   std::map< WeatherData::PROPERTY, double > upperLimit;
   std::map< WeatherData::PROPERTY, double > oldAlarmVal;
   std::map< WeatherData::PROPERTY, bool > alarming;
   Now alarmCheckTime;
};

#endif // __ALARMS_H__
