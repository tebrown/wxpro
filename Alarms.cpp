#include "Alarms.h"
#include "units.h"
#include <sstream>

Alarms::Alarms( Config cfg, WeatherData *_wd ):
   WeatherSink( cfg, _wd, "Alarms" )

{
   pthread_mutex_init( &notify, NULL );
   pthread_mutex_lock( &notify );
   startMain();
}

Alarms::~Alarms()
{
   endMain();
}

void Alarms::setLimits()
{

   for ( int p = WeatherData::insideTemp; p != WeatherData::END; p++ )
   {
      lowerLimit[ ( WeatherData::PROPERTY)p ] = NO_VALUE;
      upperLimit[ ( WeatherData::PROPERTY)p ] = NO_VALUE;
      alarming[ (WeatherData::PROPERTY)p ] = false;
   }
   lowerLimit[WeatherData::insideTemp] = 55;
   upperLimit[WeatherData::insideTemp] = 78;
   lowerLimit[WeatherData::insideHumidity] = 10;
   upperLimit[WeatherData::insideHumidity] = 90;
   lowerLimit[WeatherData::outsideTemp] = 0;
   upperLimit[WeatherData::outsideTemp] = 90;
   upperLimit[WeatherData::windSpeed] = 30;
   upperLimit[WeatherData::averageWindSpeed] = 30;
   upperLimit[WeatherData::windGust] = 30;
   upperLimit[WeatherData::dailyRain] = 100;
   lowerLimit[WeatherData::SLP] = (29.00);
   upperLimit[WeatherData::SLP] = (31.00);

   lowerLimit[WeatherData::apparentTemp] = 0;
   upperLimit[WeatherData::apparentTemp] = 90;

   upperLimit[WeatherData::rain24Hour] = 100;
   upperLimit[WeatherData::instantRain] = 100;

   // TODO: Need to add calculated values too
}


void Alarms::newData()
{
   pthread_mutex_unlock( &notify );
}

void Alarms::main()
{
   bool alarmOccured;
   setLimits();
   while ( threadRunning )
   {
      std::ostringstream oss;
      alarmOccured = false;
      pthread_mutex_lock( &notify );
      if( !threadRunning )
      {
         dbg.printf(NOTICE, "Exiting\n");
         return;
      }

      float timeSinceLast = alarmCheckTime.timeSinceLast();
      dbg.printf(DEBUG, "time since last alarm check: %f\n", timeSinceLast);

      if ( timeSinceLast <= ALARM_INTERVAL )
      {
         dbg.printf(DEBUG, "too soon since last update.. not doing anything\n");
         continue;
      }
      for ( int i = WeatherData::insideTemp; i != WeatherData::END; i++ )
      {
         WeatherData::PROPERTY p = (WeatherData::PROPERTY)i;
         // localAlarm is used in the loop to determine if we need to throw
         // another alarm condition.  It is set anytime an alarm condition is
         // encountered.  We can use this to see if we were alarming in our
         // previous iteration (in which case we don't throw the alarm), or if
         // this is a new alarm condition
         bool localAlarm = false;

         if ( lowerLimit[p] != NO_VALUE )
         {
            // If we are alarming, we won't send out another notice until this
            // alarm clears
            if ( wd->getValue( p, 0 ) < 
               lowerLimit[ p ] )
            {
               localAlarm = true;
               if ( !alarming[ p ] )
               {
                  alarming[ p] = true;
                  oss << wd->propertyToString(p);
                  oss << " exceeding alarm condition.  Current value is "; 
                  oss << wd->getValue( p, 0 ) <<  ", lower limit is ";
                  oss <<  lowerLimit[ p ] << std::endl;
                  alarmOccured = true;
               }
            }
         }

         if ( upperLimit[p] != NO_VALUE )
         {
            // If we are alarming, we won't send out another notice until this
            // alarm clears
            if ( wd->getValue( p, 0 ) > upperLimit[ p ] )
            {
               localAlarm = true;
               if ( !alarming[ p ] )
               {
                  alarming[ p ] = true;
                  oss << wd->propertyToString(p);
                  oss << " exceeding alarm condition.  Current value is "; 
                  oss << wd->getValue( p, 0 ) <<  ", upper limit is ";
                  oss <<  upperLimit[ p ] << std::endl;
                  alarmOccured = true;
               }
            }
         }
         // If this was alarming, but now we aren't...
         if ( !localAlarm && alarming[p] )
         {
            dbg.printf(NOTICE, "Resetting alarm condition on %s\n",
                  wd->propertyToString(p));
            alarming[p] = false;
         }
      }
      if ( alarmOccured )
      {
         dbg.printf(NOTICE, oss.str().c_str());
      }
      alarmCheckTime.updateTime();
   }

}

