#ifndef __WeatherBug_H__
#define __WeatherBug_H__
#include "WeatherSink.h"
#include <curl/curl.h>

#define WEATHERBUG_STRING_SIZE (512)
#define WEATHERBUG_RT_URL "http://data.backyard2.weatherbug.com/data/livedata.aspx"

class WeatherBug: public WeatherSink
{
public:
   WeatherBug( Config cfg, WeatherData *wd );
   ~WeatherBug();

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
   char uploadString[WEATHERBUG_STRING_SIZE];
};

#endif //__WeatherBug_H__
#ifndef __WeatherBug_H__
#define __WeatherBug_H__
#include "WeatherSink.h"
#include <curl/curl.h>

#define WEATHERBUG_STRING_SIZE (512)
#define WEATHERBUG_RT_URL "http://data.backyard2.weatherbug.com/data/livedata.aspx"

class WeatherBug: public WeatherSink
{
public:
   WeatherBug( Config cfg, WeatherData *wd );
   ~WeatherBug();

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
   char uploadString[WEATHERBUG_STRING_SIZE];
};

#endif //__WeatherBug_H__
