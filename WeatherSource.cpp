#include "WeatherSource.h"

WeatherSource::WeatherSource()
{
}

WeatherSource::~WeatherSource()
{
}

bool WeatherSource::read( WeatherData *_wd )
{
   wd = _wd;

   bool rval = false;
   rval = this->read();

   if ( rval )
   {
      wd->newData();
   }

   return rval;
      
}
