#include "wunderground.h"
#include "time.h"
#include <signal.h>
#include "utils.h"
#include <sys/ioctl.h>

size_t Wunderground::curl_write( void *buffer, size_t size, size_t nmemb, void* userp )
{
      return size*nmemb;
}


Wunderground::Wunderground( Config cfg, WeatherData *_wd ):
   WeatherSink( cfg, _wd, "Weather Underground" ),
   failureCount( 0 ),
   updateFrequency( 2.5 )
{
   pthread_mutex_init( &notify, NULL );
   uploadTime.updateTime();
   startMain();
}

Wunderground::~Wunderground()
{
   endMain();

}


void Wunderground::newData()
{
   pthread_mutex_unlock( &notify );
}

void Wunderground::generatePacket()
{
   struct tm gmt;
   time_t t;
   t = (time_t) wd->now->unixtime() ;
   char st[32];
   

   gmtime_r( &t, &gmt );
   strftime( st, 31,  "%Y-%m-%d+%H%%3A%M%%3A%S", &gmt );

   snprintf(uploadString, WUNDERGROUND_STRING_SIZE,
         "%s?ID=%s&PASSWORD=%s&dateutc=%s",
         WUNDERGROUND_RT_URL, 
         cfg.getString("wunderground_id").c_str(),
         cfg.getString("wunderground_password").c_str(),
         st );

   appendData( "&winddir=%.0f", WeatherData::windDirection );
   appendData( "&windspeedmph=%.2f",WeatherData::windSpeed );
   //appendData( "&windgustdir=%.0f",WeatherData::windGustDir );
   //appendData( "&windgustmph=%.2f",WeatherData::windGust );
   appendData( "&windgustdir_10m=%.0f", WeatherData::windGustDir );
   appendData( "&windgustmph_10m=%.2f", WeatherData::windGust );
   appendData( "&winddir_avg2m=%.0f", WeatherData::averageWindDirection );
   appendData( "&windspdmph_avg2m=%.2f", WeatherData::averageWindSpeed );
   appendData( "&tempf=%.2f", WeatherData::outsideTemp );
   appendData( "&rainin=%.2f", WeatherData::rainRate, 100.0 );
   appendData( "&dailyrainin=%.2f", WeatherData::dailyRain, 100.0 );
   appendData( "&baromin=%.2f", WeatherData::SLP );
   appendData( "&dewptf=%.2f", WeatherData::dewPoint );
   appendData( "&humidity=%.0f", WeatherData::outsideHumidity );
   appendData( "&solarradiation=%.0f", WeatherData::solarRadiation );
   appendData( "&UV=%.1f", WeatherData::UV );
   appendData( "&indoortempf=%.2f", WeatherData::insideTemp );
   appendData( "&indoorhumidity=%.0f", WeatherData::insideHumidity );

   strnfcat( uploadString, WUNDERGROUND_STRING_SIZE, "&softwaretype=%s%s",
         "WxPro%20",
         "DR1");

   strnfcat( uploadString, WUNDERGROUND_STRING_SIZE, "&action=updateraw&realtime=1&rtfreq=%.2f",
         updateFrequency);
}

void Wunderground::uploadPacket()
{
   if ( cfg.getInteger("test_only") != 1 )
   {
      curl = curl_easy_init();
      curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, Wunderground::curl_write);
      curl_easy_setopt( curl, CURLOPT_TIMEOUT, 5 );
      curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, curlErrorBuf );
      curl_easy_setopt( curl, CURLOPT_NOSIGNAL, 1 );
      curl_easy_setopt( curl, CURLOPT_URL, uploadString );
      dbg.printf( DEBUG, "CURL upload string: '%s'\n", uploadString );
      res=curl_easy_perform(curl);
      if ( (int)res != 0 )
      {
         failureCount++;
         if ( failureCount < 10 )
         {
            dbg.printf(DEBUG, "Could not write to wunderground.  Error: %s (%d)\n",
                  curlErrorBuf, res );
         }
         else
         {
            dbg.printf(CRIT, "Could not write to wunderground after 10 attempts.  Error: %s (%d)\n",
                  curlErrorBuf, res );
            failureCount = 0;
         }
      }
      else
      { 
         failureCount = 0;
      }
      if ( curl )
         curl_easy_cleanup( curl );
      }
   else
   {
      dbg.printf(NOTICE,"Pretending to Uploading to wunderground\n");
      dbg.printf(DEBUG, "uploadString: %s\n", uploadString );
   }
   uploadTime.updateTime();
}

void Wunderground::main()
{

   while( threadRunning )
   {
      pthread_mutex_lock( &notify );
      if ( !threadRunning )
         return;

      float timeSinceLast = uploadTime.timeSinceLast();
      dbg.printf(DEBUG, "time since last: %f\n", timeSinceLast);
      if ( timeSinceLast <= 2 )
      {
         dbg.printf(DEBUG, "too soon since last update.. not doing anything\n");
         continue;
      }
      if ( timeSinceLast > 2 && timeSinceLast < updateFrequency )
      {
         int sleepTime = (int)((updateFrequency - timeSinceLast ) * 1000000 );
         dbg.printf(DEBUG, "too soon since last update.. sleeping for %d usec\n", sleepTime);
         usleep( sleepTime );
      }

      generatePacket();
      uploadPacket();
   }
}


bool Wunderground::appendData( const char *format, WeatherData::PROPERTY property, float divider )
{
   char buf[255];
   if ( wd->hasData( property ) )
   {
      snprintf(buf, 255, format, wd->getValue( property, 0 ) / divider );
      if ( strlen(buf) + strlen( uploadString ) >= WUNDERGROUND_STRING_SIZE )
      {
         dbg.printf(CRIT, "Buffer size exceeded.\n");
         return false;
      }
      strcat( uploadString, buf );
      return true;
   }
   return false;

}
