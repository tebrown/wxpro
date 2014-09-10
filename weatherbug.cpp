#include "weatherbug.h"
#include "time.h"
#include <signal.h>
#include "utils.h"
#include <sys/ioctl.h>

size_t WeatherBug::curl_write( void *buffer, size_t size, size_t nmemb, void* userp )
{
      return size*nmemb;
}


WeatherBug::WeatherBug( Config cfg, WeatherData *_wd ):
   WeatherSink( cfg, _wd, "WeatherBug" ),
   failureCount( 0 ),
   updateFrequency( 60 )
{
   pthread_mutex_init( &notify, NULL );
   uploadTime.updateTime();
   startMain();
}

WeatherBug::~WeatherBug()
{
   endMain();

}


void WeatherBug::newData()
{
   pthread_mutex_unlock( &notify );
}

void WeatherBug::generatePacket()
{
   struct tm gmt;
   time_t t;
   t = (time_t) wd->now->unixtime() ;
   char st[32];
   

   gmtime_r( &t, &gmt );
   strftime( st, 31,  "%Y-%m-%d+%H%%3A%M%%3A%S", &gmt );

   snprintf(uploadString, WEATHERBUG_STRING_SIZE,
         "%s?ID=%s&Key=%s&num=%s&dateutc=%s",
         WEATHERBUG_RT_URL, 
         cfg.getString("weatherbug_id").c_str(),
         cfg.getString("weatherbug_key").c_str(),
         cfg.getString("weatherbug_num").c_str(),
         st );

   appendData( "&winddir=%.0f", WeatherData::windDirection );
   appendData( "&windspeedmph=%.2f",WeatherData::averageWindSpeed );
   appendData( "&windgustmph=%.2f", WeatherData::windGust );
   appendData( "&humidity=%.0f", WeatherData::outsideHumidity );
   appendData( "&tempf=%.2f", WeatherData::outsideTemp );
   appendData( "&rainin=%.2f", WeatherData::rainRate, 100.0 );
   appendData( "&dailyrainin=%.2f", WeatherData::dailyRain, 100.0 );
   appendData( "&baromin=%.2f", WeatherData::SLP );

   strnfcat( uploadString, WEATHERBUG_STRING_SIZE, "&softwaretype=%s%s",
         "WxPro%20",
         "DR1");

}

void WeatherBug::uploadPacket()
{
   if ( cfg.getInteger("test_only") != 1 )
   {
      curl = curl_easy_init();
      curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WeatherBug::curl_write);
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
            dbg.printf(DEBUG, "Could not write to WeatherBug.  Error: %s (%d)\n",
                  curlErrorBuf, res );
         }
         else
         {
            dbg.printf(CRIT, "Could not write to WeatherBug after 10 attempts.  Error: %s (%d)\n",
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
      dbg.printf(NOTICE,"Pretending to Uploading to WeatherBug\n");
      dbg.printf(DEBUG, "uploadString: %s\n", uploadString );
   }
   uploadTime.updateTime();
}

void WeatherBug::main()
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
         int sleepTime = (int)((updateFrequency - timeSinceLast ) );
         dbg.printf(DEBUG, "too soon since last update.. sleeping for %d usec\n", sleepTime);
         for ( int i = 0; i < sleepTime; i++)
         {
            sleep(1);
            if ( !threadRunning )
               return;
         }
      }

      generatePacket();
      uploadPacket();
   }
}


bool WeatherBug::appendData( const char *format, WeatherData::PROPERTY property, float divider )
{
   char buf[255];
   if ( wd->hasData( property ) )
   {
      snprintf(buf, 255, format, wd->getValue( property, 0 ) / divider );
      if ( strlen(buf) + strlen( uploadString ) >= WEATHERBUG_STRING_SIZE )
      {
         dbg.printf(CRIT, "Buffer size exceeded.\n");
         return false;
      }
      strcat( uploadString, buf );
      return true;
   }
   return false;

}
