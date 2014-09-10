#ifndef __WEATHER_SOURCE_H__
#define __WEATHER_SOURCE_H__

#include "WeatherData.h"
#include "Config.h"

class WeatherSource
{
   public:
      WeatherSource( Config &cfg );
      WeatherSource( );

      virtual ~WeatherSource();
      bool read( WeatherData *wd );
      
   public:
      virtual bool read() = 0;

   protected:
      WeatherData *wd;
};

#endif // __WEATHER_SOURCE_H__
