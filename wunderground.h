#ifndef __WUNDERGROUND_H__
#define __WUNDERGROUND_H__
#include "WeatherSink.h"
#include <curl/curl.h>

#define WUNDERGROUND_STRING_SIZE (512)
#define WUNDERGROUND_RT_URL "http://rtupdate.wunderground.com/weatherstation/updateweatherstation.php"

class Wunderground: public WeatherSink
{
public:
   Wunderground( Config cfg, WeatherData *wd );
   ~Wunderground();

   void newData();
   void main();

   static size_t curl_write( void *buffer, size_t size, size_t nmemb, void* userp );

private:
   bool appendData( const char * format, WeatherData::PROPERTY, float divider = 1 );
   void generatePacket( void );
   void uploadPacket( void );

private:
   pthread_mutex_t notify;
   CURL *curl;
   CURLcode res;
   int failureCount;
   float updateFrequency;
   Now uploadTime;

   char curlErrorBuf[CURL_ERROR_SIZE];
   char uploadString[WUNDERGROUND_STRING_SIZE];
};




#endif //__WUNDERGROUND_H__
