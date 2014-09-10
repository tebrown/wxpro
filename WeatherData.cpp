#include "WeatherData.h"
#include "units.h"
#include "math.h"
#include "WeatherSink.h"

/* Solar Constant - The solar constant is the amount of energy received at the
 * top of the Earth's atmosphere on a surface oriented perpendicular to the
 * Sun.s rays (at the mean distance of the Earth from the Sun). The generally
 * accepted solar constant of 1368 W/m2 is a satellite measured yearly
 * average. */
/* http://edmall.gsfc.nasa.gov/inv99Project.Site/Pages/science-briefs/ed-stickler/ed-irradiance.html
 */
#define MAX_SOLAR (1368)

using namespace std;


WeatherData::WeatherData( Config _cfg ):
   now( new Now ),
   cfg( _cfg ),
   db(),
   wind( now, db ),
   rain( now, db ),
   dbg("Weather Data"),
   baro(cfg.getDouble("elevation")),
   almanacYesterday( cfg, now->unixtime() - 24*60*60 ),
   almanacToday( cfg, now->unixtime() ),
   almanacTomorrow( cfg, now->unixtime() + 24*60*60 ),
   dayTime( false )
{
   pthread_rwlock_init( &rwLock, 0 );
   for ( int p = insideTemp; p != END; p++ )
   {
      dataAvailable[ (PROPERTY)p ] = false;
      setValues[ (PROPERTY)p ] = NO_VALUE;
   }
   inst[insideTemp].setHistorySize( 5 * ONE_MINUTE, "insideTemp" );
   inst[insideHumidity].setHistorySize( 1 * ONE_MINUTE, "insideHumidity" );
   inst[outsideTemp].setHistorySize( 5 * ONE_MINUTE, "outsideTemp" );
   inst[outsideHumidity].setHistorySize( 1 * ONE_MINUTE, "outsideHumidity" );
   refreshSolarMax();
   computeTrends();

   time_t t;
   t = almanacYesterday.sunrise;
   dbg.printf(NOTICE, "Sunrise yesterday:  %s", ctime(&t));
   t = almanacYesterday.sunset;
   dbg.printf(NOTICE, "Sunset yesterday:  %s", ctime(&t));

   t = almanacToday.sunrise;
   dbg.printf(NOTICE, "Sunrise today:  %s", ctime(&t));
   t = almanacToday.sunset;
   dbg.printf(NOTICE, "Sunset today:  %s", ctime(&t));

   t = almanacTomorrow.sunrise;
   dbg.printf(NOTICE, "Sunrise tomorrow:  %s", ctime(&t));
   t = almanacTomorrow.sunset;
   dbg.printf(NOTICE, "Sunset Tomorrow:  %s", ctime(&t));

   if ( almanacToday.lengthOfDay < almanacYesterday.lengthOfDay  )
   {
      dbg.printf(NOTICE, "Today will be %d seconds shorter than yesterday\n",
            almanacYesterday.lengthOfDay - almanacToday.lengthOfDay );
   } 
   else if ( almanacToday.lengthOfDay > almanacYesterday.lengthOfDay  )
   {
      dbg.printf(NOTICE, "Today will be %d seconds longer than yesterday\n",
            almanacToday.lengthOfDay - almanacYesterday.lengthOfDay );
   }
   else 
   {
      dbg.printf(NOTICE, "Today will be same length as yesterday\n");
   }

   if ( almanacTomorrow.lengthOfDay < almanacToday.lengthOfDay  )
   {
      dbg.printf(NOTICE, "Tomorrow will be %d seconds shorter than today\n",
            almanacToday.lengthOfDay - almanacTomorrow.lengthOfDay );
   } 
   else if ( almanacTomorrow.lengthOfDay > almanacToday.lengthOfDay  )
   {
      dbg.printf(NOTICE, "Tomorrow will be %d seconds longer than today\n",
            almanacTomorrow.lengthOfDay - almanacToday.lengthOfDay );
   }
   else 
   {
      dbg.printf(NOTICE, "Tomorrow will be same length as today\n");
   }

   if ( now->unixtime() > almanacToday.sunrise  &&
         now->unixtime() < almanacToday.sunset )
   {
      dbg.printf(NOTICE, "Initializing in daytime mode\n");
      dayTime = true;
   }
   else
   {
      dbg.printf(NOTICE, "Initializing in nighttime mode\n");
      dayTime = false;
   }
} 

WeatherData::~WeatherData()
{
}


double WeatherData::getCurrent( PROPERTY property )
{
   if ( !dataAvailable[ property ] )
   {
      dbg.printf(INFO, "No data available for %s\n",
            propertyToString(property));
      return NO_VALUE;
   }
   return inst[property];
}

double WeatherData::getFromObs( PROPERTY property, time_t howLongAgo, time_t
      *actualTime )
{
   // Bracket it between +- 30 seconds.  This should give us a good time.
   dbResult result = db.query("select time, %s from dailyObs where time < %d and time > %d order by time desc limit 1;", 
         propertyToString(property), 
         (int)howLongAgo + 30, 
         (int)howLongAgo - ONE_MINUTE*5 );

   if ( result.size() == 0 )
   {
      return NO_VALUE;
   }
   else
   {
      if ( actualTime != NULL )
         *actualTime = result[0].asInteger("time");
      return result[0].asDouble(propertyToString(property));
   }
}

double WeatherData::getFromHourly( PROPERTY property, time_t howLongAgo,
      time_t *actualTime )
{
   dbResult result = db.query("select time, %s from hourlySummary where time < %d and time > %d order by time desc limit 1;", 
         propertyToString(property), 
         (int)howLongAgo + ONE_HOUR/2, 
         (int)howLongAgo - ONE_HOUR*5 );

   if ( result.size() == 0 )
   {
      return NO_VALUE;
   }
   else
   {
      if ( actualTime != NULL )
         *actualTime = result[0].asInteger("time");
      return result[0].asDouble(propertyToString(property));
   }
}

double WeatherData::getFromDaily( PROPERTY property, time_t howLongAgo, 
      time_t *actualTime )
{
   dbResult result = db.query("select time, %s from dailySummary where time < %d and time > %d and summaryType == 0 order by time desc limit 1;", 
         propertyToString(property), 
         (int)howLongAgo + ONE_DAY/2, 
         (int)howLongAgo - ONE_DAY/2 );

   if ( result.size() == 0 )
   {
      return NO_VALUE;
   }
   else
   {
      if ( actualTime != NULL )
         *actualTime = result[0].asInteger("time");
      return result[0].asDouble(propertyToString(property));
   }
}

void WeatherData::copyData( void )
{
   for ( int p = insideTemp; p != END; p++ )
   {
      PROPERTY property = (PROPERTY)p;
      if ( setValues[ property ] != NO_VALUE )
      {
         inst[ property ] = setValues[ property ];
         dataAvailable[ property ] = true;
      }
   }

}

bool WeatherData::setValue( PROPERTY property, double value )
{
   bool rval = false;
   switch ( property )
   {
      case windGust:
      case windGustDir:
      case solarPercent:
      case rain24Hour:
      case rainRate:
         dbg.printf(CRIT, "windGust/solarPecent are not settable!\n");
         rval = false;
         break;
      default:
         setValues[property] = value;
         rval = true;
         break;
   }
   return rval;
}

double WeatherData::getTrend( PROPERTY property )
{
   pthread_rwlock_rdlock( &rwLock );
   double f = trend[property];
   pthread_rwlock_unlock( &rwLock );
   return f;
}

double WeatherData::getValue( PROPERTY property, time_t howLongAgo, time_t
      *actualTime )
{
   double rval = 0;
   pthread_rwlock_rdlock( &rwLock );
   if ( howLongAgo == 0 )
      rval = getCurrent( property );
   else if ( howLongAgo < ONE_DAY*7 )
      rval = getFromObs( property, now->unixtime() - howLongAgo, actualTime );
   else if ( howLongAgo < ONE_DAY*90 )
      rval = getFromHourly( property, now->unixtime() - howLongAgo, actualTime);
   else 
      rval = getFromDaily( property, now->unixtime() - howLongAgo, actualTime);
   pthread_rwlock_unlock( &rwLock );

   return rval;
}

double WeatherData::getSolarMax()
{
   return currentSolarMax;
}

double WeatherData::getApparentTemp()
{
   // Humidex is valid for temps > 68
   if ( (double)inst[humidex] != NO_VALUE )
   {
      return inst[humidex];
   }
   // Windchill is valid for temps < 50 and > -50
   else if ( (double)inst[windChill] != NO_VALUE )
   {
      return inst[windChill];
   }
   // Otherwise, there is no effect on the human body
   else 
   {
      return inst[outsideTemp];
   }
}

double WeatherData::getCloudHeight()
{
   return trunc((( (float)inst[outsideTemp] - (float)inst[dewPoint])/4.5)*1000);
}


double WeatherData::getStationPressure()
{
   int meanTemp;
   float oldTemp = getFromObs( outsideTemp, now->unixtime()-12*ONE_HOUR, NULL );
   if ( oldTemp == NO_VALUE )
   {
      dbg.printf(NOTICE, "No temperature data for 12 hours ago. Using current temp only.\n");
      meanTemp = (int)inst[outsideTemp];
   }
   else
   {
      meanTemp = ((int)oldTemp + (int)inst[outsideTemp]) / 2;
   }

   return baro.SLPtoStationPressure( inst[SLP], inst[outsideTemp],
         (float)meanTemp, (int)inst[outsideHumidity] ) + cfg.getDouble("barometer_offset");

}

double WeatherData::getHeatIndex()
{
   double T = inst[outsideTemp];
   double R = inst[outsideHumidity];

   if ( T < 68 || R < 40 )
   {
      return NO_VALUE;
   }               

   double T2 = pow(T,2);
   double R2 = pow(R,2);
   return  -42.379
                   +(2.04901523*T)
                   +(10.14333127*R)
                   -(0.22475541*T*R)
                   -(0.00683783*T2)
                   -(0.05481717*R2)
                   +(0.00122874*T2*R)
                   +(0.00085282*T*R2)
                   -(0.000001998*T2*R2);
                   

}

double WeatherData::getHumidex()
{
   if ( (double)inst[outsideTemp] < 68 || (double)inst[outsideHumidity] < 40 )
   {
      return NO_VALUE;
   }               
   double temp = F2C(inst[outsideTemp]);
   double dp = F2K( inst[dewPoint] );
   double e = 6.11 * exp(5417.7530 * ((1/273.16)-(1/dp)));
   double h = (0.5555)*(e - 10.0 );
   return C2F( temp + h );

}

double WeatherData::getWindChill()
{
   double v = inst[averageWindSpeed];
   double T = inst[outsideTemp];
   if ( ( v < 3 ) || T > 50 || T < -50 )
      return NO_VALUE;

   return ( 35.74 + 0.6215*T - 35.75*pow(v, 0.16) + 0.4275*T*pow(v, 0.16 ));
}

double WeatherData::getDewPoint()
{
   double temp = F2C(inst[outsideTemp]);
   double humidity = (double)inst[outsideHumidity]/100.0;

   double a = 17.27;
   double b = 237.7;
   double theta = ((a*temp)/(b+temp))+log(humidity);

   return C2F((( b * theta )/(a - theta )));

}

bool WeatherData::newData()
{
   pthread_rwlock_wrlock( &rwLock );
   copyData();

   if ( dataAvailable[dailyRain] )
   {
      rain.update( (int)(inst[dailyRain]) );
      inst[rainRate] = rain.getRate();
      inst[rain24Hour] = rain.getSum( 24 * ONE_HOUR );
      inst[instantRain] = rain.getInstantRainrate();
      dataAvailable[rainRate] = true;
      dataAvailable[rain24Hour] = true;
      dataAvailable[instantRain] = true;
   }

   if ( dataAvailable[ windSpeed ] && dataAvailable[ windDirection ] )
   {
      wind.update( inst[windSpeed], inst[windDirection] );
      inst[averageWindDirection] = wind.getAverageDirection( TWO_MINUTES );
      inst[averageWindSpeed] = wind.getAverageSpeed( TWO_MINUTES );
      inst[windGust] = wind.getMaxSpeed( ONE_MINUTE * 10 );
      inst[windGustDir] = wind.getDirectionAtMaxSpeed( ONE_MINUTE * 10 );

      dataAvailable[averageWindDirection] = true;
      dataAvailable[averageWindSpeed] = true;
      dataAvailable[windGust] = true;
      dataAvailable[windGustDir] = true;
   }

   // We were in daytime mode, but time has transitions to night (We are past
   // sunset).
   if ( dayTime && now->unixtime() > almanacToday.sunset )
   {
      dbg.printf(NOTICE, "Transitioning to nighttime mode\n");
      dayTime = false;
      updateSummary( DAILY, DAYTIMESUMMARY );

   }
   // We were in night mode, but are now in daytime ( sunrise HAS happened,
   // but sunset hasn't)
   else if ( !dayTime && 
         now->unixtime() > almanacToday.sunrise &&
         now->unixtime() < almanacToday.sunset )
   {
      dbg.printf(NOTICE, "Transitioning to daytime mode\n");
      dayTime = true;
      updateSummary( DAILY, NIGHTTIMESUMMARY );
   }
   updateDB();
   pthread_rwlock_unlock( &rwLock );


   if ( now->isNewDay() )
   {
      dbg.printf(EMERG, "refreshing almanacs\n");
      refreshAlmanacs();
   }
   /*for ( std::vector< WeatherSink *>::iterator iter = sinks.begin(); 
         iter != sinks.end(); ++iter )
   {
      (*iter)->newData();
   }*/
   return true;
}

double WeatherData::computeTrend( PROPERTY property, int duration /* hours */)
{

   const char * key = propertyToString( property );
   int age = (int)(now->unixtime() - duration*ONE_HOUR );
   
   dbResult result = db.query(
         " select ( "
         "  select sum( %s *("
         "   select time/3600.0 - ("
         "    select avg(time/3600.0) from dailyObs where time > %d) )) "
         "   from dailyObs where time > %d ) "
         "  / "
         "   ( select sum((time/3600.0 - ( "
         "    select avg(time/3600.0) from dailyObs where time > %d)) * "
         "     (time/3600.0 - ( "
         "       select avg(time/3600.0) from dailyObs where time > %d))) "
         "   from dailyObs where time > %d) as M;", 
         key, age, age, age, age, age );

   if ( result.size() == 0 )
      return 0;
   else
      return result[0].asDouble("M");
}

void WeatherData::doMaintenance()
{
   static bool firstMaintenanceRun = true;
   if ( !now->isNewMinute( ) && !firstMaintenanceRun )
   {
      return;
   }

   firstMaintenanceRun = false;

   if ( dataAvailable[SLP] )
   {
      inst[stationPressure] = getStationPressure();
      inst[altimeter]  = baro.StationPressureToAltimeter( inst[stationPressure] );
      dataAvailable[ stationPressure ] = true;
      dataAvailable[ altimeter ] = true;

      dbg.printf(INFO, "             SLP: %f (%.1f)\n", 
            (float)inst[SLP], inHg2hPa( inst[SLP] ) );
      dbg.printf(INFO, "station pressure: %f (%.1f)\n", 
            (float)inst[stationPressure], inHg2hPa( inst[stationPressure] ) );
      dbg.printf(INFO, "       altimeter: %f (%.1f)\n", 
            (float)inst[altimeter], inHg2hPa( inst[ altimeter] ) );
   }
   refreshSolarMax();
   computeTrends();
}

void WeatherData::updateDB()
{
   double sradMax = getSolarMax();
   int sradPercent = 0;
   if ( sradMax > 0 )
      sradPercent = (int)round( ( (double)inst[solarRadiation] / sradMax) * 100 );
   if ( sradPercent > 100 )
      sradPercent = 100;

   if ( dataAvailable[ solarRadiation ] )
   {
      dataAvailable[ solarPercent ] = true;
      inst[solarPercent] = sradPercent;
   }

   if ( dataAvailable[ outsideTemp ] && dataAvailable[ outsideHumidity ] )
   {
      inst[dewPoint] = getDewPoint();
      inst[humidex] = getHumidex();
      inst[heatIndex] = getHeatIndex();

      // cloud height depends on dew point, so it should come after that
      inst[cloudHeight] = getCloudHeight();

      dataAvailable[heatIndex] = true;
      dataAvailable[cloudHeight] = true;
      dataAvailable[humidex] = true;
      dataAvailable[dewPoint] = true;
   }

   if ( dataAvailable[ outsideTemp ] && dataAvailable[ windSpeed ] )
   {
      inst[windChill] = getWindChill();
      dataAvailable[windChill] = true;
   }

   if ( dataAvailable[ humidex ] && dataAvailable[ windChill ] )
   {
      // Apparent Temp depends on windchill and heat index, so it should come
      // after those
      inst[apparentTemp] = getApparentTemp();
      dataAvailable[apparentTemp] = true;
   }

   doMaintenance(); 

   if ( !now->isNewMinute( ))
   {
      return;
   }



   dbg.printf(DEBUG, "Logging minute summary\n");
   db.query("delete from dailyObs where time < %d;", (int)(now->unixtime() - ONE_DAY*7 ));
   db.query("insert into dailyObs ( time, insideTemp, insideHumidity, outsideTemp, outsideHumidity, windSpeed, windDirection, windGust, rainRate, dailyRain, dailyET, UV, solarRadiation, solarRadiationPercent, barometricTrend, rawBarometer, SLP, altimeter, dewPoint, heatIndex, humidex, windChill, apparentTemp, cloudHeight, windGustDir, rain24Hour, instantRain ) VALUES ( "
         "%d, "   // unixTime
         "%.02f," // insideTemp
         "%.0f,"  // insideHumidity
         "%.02f," // outsideTemp
         "%.0f,"  // outsideHumidity
         "%.02f," // averageWindSpeed
         "%.0f,"  // averageWindDirection
         "%.02f," // windGust
         "%.02f," // rainRate
         "%.02f," // dailyRain
         "%.02f," // dailyET
         "%.02f," // UV
         "%.0f,"  // solarRadiation
         "%.0f,"  // solarPercent
         "%.04f," // barometer trend
         "%.02f," // barometer
         "%.02f," // SLP
         "%.02f," // altimeter
         "%.02f," // dewPoint
         "%.02f," // heatIndex
         "%.02f," // humidex
         "%.02f," // windChill
         "%.02f," // apparentTemp
         "%.0f,"  // cloudHeight
         "%.0f,"  // windGustDir
         "%.02f," // rain24Hour
         "%.02f);", // instantRain
         (int)now->unixtime(),
         (float)inst[insideTemp],
         (float)inst[insideHumidity],
         (float)inst[outsideTemp],
         (float)inst[outsideHumidity],
         (float)inst[averageWindSpeed],
         (float)(float)inst[averageWindDirection],
         (float)inst[windGust],
         (float)inst[rainRate],
         (float)inst[dailyRain],
         (float)inst[dailyET],
         (float)inst[UV],
         (float)inst[solarRadiation],
         (float)inst[solarPercent],
         trend[altimeter],
         (float)inst[stationPressure],
         (float)inst[SLP],
         (float)inst[altimeter],
         (float)inst[dewPoint],
         (float)inst[heatIndex],
         (float)inst[humidex],
         (float)inst[windChill],
         (float)inst[apparentTemp],
         (float)inst[cloudHeight],
         (float)inst[windGustDir],
         (float)inst[rain24Hour],
         (float)inst[instantRain]
      );

   if ( now->getMinute() != 59 )
   {
      return;
   }
   dbg.printf(DEBUG, "Logging hourly summary\n");

   db.query("delete from hourlySummary where time < %d;", (int)(now->unixtime() - ONE_DAY*90 ));
   updateSummary( HOURLY );

   if ( now->getHour() != 23 )
   {
      return;
   }
   dbg.printf(DEBUG, "Logging daily summary\n");

   updateSummary( DAILY );
   db.query("VACUUM;");
}

void WeatherData::refreshAlmanacs()
{
   dbg.printf(NOTICE, "Refreshing almanacs\n");
   almanacYesterday.setNewTime(now->unixtime() - 12*60*60 );   
   almanacToday.setNewTime(now->unixtime());
   almanacTomorrow.setNewTime(now->unixtime() + 36*60*60);

   time_t t;
   t = almanacYesterday.sunrise;
   dbg.printf(NOTICE, "Sunrise yesterday:  %s", ctime(&t));
   t = almanacYesterday.sunset;
   dbg.printf(NOTICE, "Sunset yesterday:  %s", ctime(&t));

   t = almanacToday.sunrise;
   dbg.printf(NOTICE, "Sunrise today:  %s", ctime(&t));
   t = almanacToday.sunset;
   dbg.printf(NOTICE, "Sunset today:  %s", ctime(&t));

   t = almanacTomorrow.sunrise;
   dbg.printf(NOTICE, "Sunrise tomorrow:  %s", ctime(&t));
   t = almanacTomorrow.sunset;
   dbg.printf(NOTICE, "Sunset Tomorrow:  %s", ctime(&t));

}

void WeatherData::updateSummary( DBTable table, SUMMARYTYPE type )
{
   const char *table_s;
   int currentTime = now->unixtime();
   int startTime;
   const char *keys[] = {"insideTemp",
                         "insideHumidity",
                         "outsideTemp",
                         "outsideHumidity",
                         "windSpeed",
                         "windGust",
                         "UV",
                         "solarRadiation",
                         "solarRadiationPercent",
                         "SLP",
                         "rainRate",
                         "altimeter",
                         "rawBarometer",
                         "dewPoint",
                         "humidex",
                         "heatIndex",
                         "windChill",
                         "apparentTemp",
                         "cloudHeight",
                         "rain24Hour"};


   switch( table )
   {
      case HOURLY:
         table_s = "hourlySummary";
         startTime = currentTime - ONE_HOUR;
         if ( type != DAILYSUMMARY )
         {
            dbg.printf(ERR, "Cannot set type for hourlySummary!\n");
            type = DAILYSUMMARY;
         }
         db.query("insert into %s ( time ) VALUES ( %d );", 
               table_s,
               (int)now->unixtime());
         break;
      case DAILY:
         table_s = "dailySummary";
         switch ( type )
         {
            case DAILYSUMMARY:
               startTime = currentTime - ONE_DAY;
               break;
            case DAYTIMESUMMARY:
               startTime = almanacToday.sunrise;
               break;
            case NIGHTTIMESUMMARY:
               startTime = almanacYesterday.sunset;
               break;
         }
         db.query("insert into %s ( time, summaryType ) VALUES ( %d, %d );", 
               table_s,
               (int)now->unixtime(),
               type);
         break;
   }

   db.query("BEGIN TRANSACTION;");

   // TODO: We need to check if this fails, and do something if it does


   for ( size_t i = 0; i < sizeof( keys )/sizeof( char * ); i++ )
   {
      db.query("update %s set %s%s = ( select %s( %s ) from dailyObs where time > %d ), %s%sTime = ( select time from dailyObs where %s = ( select %s( %s ) from dailyObs where time > %d )) where time = %d;",
            table_s,
            keys[i],
            "min",
            "min",
            keys[i],
            startTime,
            keys[i],
            "min",
            keys[i], 
            "min",
            keys[i],
            startTime,
            currentTime );
      db.query("update %s set %s%s = ( select %s( %s ) from dailyObs where time > %d ), %s%sTime = ( select time from dailyObs where %s = ( select %s( %s ) from dailyObs where time > %d )) where time = %d;",
            table_s,
            keys[i],
            "max",
            "max",
            keys[i],
            startTime,
            keys[i],
            "max",
            keys[i], 
            "max",
            keys[i],
            startTime,
            currentTime );

      db.query("update %s set %s%s = ( select %s( %s ) from dailyObs where time > %d ) where time = %d;",
            table_s,
            keys[i],
            "Avg",
            "avg",
            keys[i],
            startTime,
            currentTime );
   }
   
   // Update instantRain
   db.query("update %s set instantRainMax = ( select max(instantRain) from dailyObs where time > %d ), instantRainMaxTime = ( select time from dailyObs where instantRain = ( select max( instantRain ) from dailyObs where time > %d ))  where time = %d\n", table_s, currentTime - 60*5, currentTime - 60*5, currentTime);

   // Update dailyrain/ET
   db.query("update %s set dailyRain = ( select max(dailyRain) from dailyObs where time > %d ) where time = %d\n", table_s, currentTime - 60*5, currentTime);
   db.query("update %s set dailyET = ( select max(dailyET) from dailyObs where time > %d ) where time = %d\n", table_s, currentTime - 60*5, currentTime);

   db.query("COMMIT;");
   
   // Update wind direction
   double windDirSin = 0;
   double windDirCos = 0;
   dbResult result = 
      db.query("select windDirection from dailyObs where time > %d;", 
            startTime );

   for ( dbResult::iterator iter = result.begin(); iter!=result.end();++iter )
   {
      windDirSin = sin( deg2rad( iter->second.asDouble("windDirection")));
      windDirCos = cos( deg2rad( iter->second.asDouble("windDirection")));
   }
   int avgWindDir = (int)rad2deg( atan2(windDirSin, windDirCos ));
   if ( avgWindDir < 0 )
      avgWindDir += 360;
   db.query("update %s set windDirectionAvg=%d where time = %d;",
         table_s, avgWindDir, currentTime );
   
   // Update wind gust direction
   windDirSin = 0;
   windDirCos = 0;
   result = 
      db.query("select windGustDir from dailyObs where time > %d;", 
            startTime );

   for ( dbResult::iterator iter = result.begin(); iter!=result.end();++iter )
   {
      windDirSin = sin( deg2rad( iter->second.asDouble("windGustDir")));
      windDirCos = cos( deg2rad( iter->second.asDouble("windGustDir")));
   }
   avgWindDir = (int)rad2deg( atan2(windDirSin, windDirCos ));
   if ( avgWindDir < 0 )
      avgWindDir += 360;
   db.query("update %s set windGustDirAvg=%d where time = %d;",
         table_s, avgWindDir, currentTime );
}


double WeatherData::refreshSolarMax()
{
   static double timezone = cfg.getDouble("timezone");
   static double longitude = cfg.getDouble("longitude");
   static double latitude = cfg.getDouble("latitude");

   float LSTM = 15 * timezone;
   float B = 360.0/365.0 * ( now->getDayOfYear() - 81 );
   float EoT = 9.8 * sin ( deg2rad(2* B)) - 7.53 * cos( deg2rad(B)) - 1.5 * sin( deg2rad( B ));
   float TC = ( -4 * ( LSTM - longitude ) + EoT ) / 60.0;
   float LT = now->getDecimalTime() - now->isDST();
   float LST = LT + TC;

   float solDec = 23.45 * sin( deg2rad( 360.0/365.0 * (284 + now->getDayOfYear())));
   float hourAngle = 15 * ( LST - 12 );
   float l = sin( deg2rad( latitude ))*sin(deg2rad(solDec));
   float r = cos( deg2rad( latitude ))*cos(deg2rad(solDec))*cos(deg2rad(hourAngle));
   float Z = acos( l + r );

   currentSolarMax = MAX_SOLAR * cos(( Z ));
   return currentSolarMax;
}

void WeatherData::computeTrends()
{
   trend[SLP] = computeTrend(SLP, 3 );
   trend[stationPressure] = computeTrend(stationPressure, 3 );
   trend[altimeter] = computeTrend(altimeter, 3 );
   trend[outsideTemp] = computeTrend(outsideTemp, 1 );
   trend[outsideHumidity] = computeTrend(outsideHumidity, 1 );
   trend[dewPoint] = computeTrend(dewPoint, 1 );
}

const char *WeatherData::propertyToString( PROPERTY property )
{
   switch ( property )
   {
      case insideTemp:
         return "insideTemp";
      case insideHumidity:
         return "insideHumidity";
      case outsideTemp:
         return "outsideTemp";
      case outsideHumidity:
         return "outsideHumidity";
      case windSpeed:
         return "windSpeed";
      case windDirection:
         return "windDirection";
      case averageWindDirection:
         return NULL;
      case averageWindSpeed:
         return NULL;
      case windGust:
         return "windGust";
      case windGustDir:
         return "windGustDir";
      case dailyRain:
         return "dailyRain";
      case rainRate:
         return "rainRate";
      case dailyET:
         return "dailyET";
      case UV:
         return "UV";
      case solarRadiation:
         return "solarRadiation";
      case solarPercent:
         return "solarRadiationPercent";
      case stationPressure:
         return "rawBarometer";
      case SLP:
         return "SLP";
      case altimeter:
         return "altimeter";
      case dewPoint:
         return "dewPoint";
      case heatIndex:
         return "heatIndex";
      case humidex: 
         return "humidex";
      case windChill:
         return "windChill";
      case apparentTemp:
         return "apparentTemp";
      case cloudHeight:
         return "cloudHeight";
      case rain24Hour:
         return "rain24Hour";
      case instantRain:
         return "instantRain";
      case END:
         return NULL;
   }
   return NULL;

}

bool WeatherData::hasData( PROPERTY property )
{
   pthread_rwlock_rdlock( &rwLock );
   bool rval= dataAvailable[ property ];
   pthread_rwlock_unlock( &rwLock );
   return rval;
}
