#ifndef __PWSWeather_H__
#define __PWSWeather_H__
#include "WeatherSink.h"
#include <curl/curl.h>

#define PWSWEATHER_STRING_SIZE (512)
#define PWSWEATHER_RT_URL "http://www.pwsweather.com/pwsupdate/pwsupdate.php"

class PWSWeather: public WeatherSink
{
public:
   PWSWeather( Config cfg, WeatherData *wd );
   ~PWSWeather();

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
   char uploadString[PWSWEATHER_STRING_SIZE];
};

#endif //__PWSWeather_H__
#ifndef __PWSWeather_H__
#define __PWSWeather_H__
#include "WeatherSink.h"
#include <curl/curl.h>

#define PWSWEATHER_STRING_SIZE (512)
#define PWSWEATHER_RT_URL "http://www.pwsweather.com/pwsupdate/pwsupdate.php"

class PWSWeather: public WeatherSink
{
public:
   PWSWeather( Config cfg, WeatherData *wd );
   ~PWSWeather();

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
   char uploadString[PWSWEATHER_STRING_SIZE];
};

#endif //__PWSWeather_H__
