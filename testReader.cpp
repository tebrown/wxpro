#include "testReader.h"
#include "utils.h"
#include "units.h"

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include "WeatherSource.h"

TestReader::TestReader( Config &cfg ):
    dbg("TestReader")
{
   if ( cfg.getInteger("test_only") != 1 )
   {
      printf("Cannot use testReader except in TEST_ONLY mode!\n");
      exit(0);
   }
}

bool TestReader::read( )
{
    wd->setValue(WeatherData::SLP, 30.12 );
    wd->setValue(WeatherData::insideTemp, 68.0 );
    wd->setValue(WeatherData::insideHumidity, 35 );
    
    wd->setValue(WeatherData::outsideTemp, 54.1 );
    wd->setValue(WeatherData::outsideHumidity, 88 );

    wd->setValue(WeatherData::windSpeed, 12.1 );
    wd->setValue(WeatherData::windDirection, 173 );

    wd->setValue(WeatherData::dailyRain, .01 );

    wd->setValue(WeatherData::dailyET, .03 );

    wd->setValue(WeatherData::solarRadiation, 1023 );
    wd->setValue(WeatherData::UV, 7.1 );

    return true;
}

