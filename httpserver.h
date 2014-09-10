#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__
#include "WeatherSink.h"

#define MAX_HTTP_BUF (1024)


class HTTPServer:public WeatherSink
{
public:
   HTTPServer( Config cfg, WeatherData *wd );
   ~HTTPServer();

   void worker( int client );
   void main( void );
   void newData() {};

private:
   void processRequest( FILE *fp, std::string req );
   std::string getCurrent();
   int outputXMLFile( FILE *outFp, const char *filename );
   void return404( FILE *fp, std::string url );

private:
   char buffer[MAX_HTTP_BUF];
};

typedef struct 
{
   HTTPServer *server;
   int client;
} HTTPThread_t;

extern "C" {
   void *startHTTPThread( void * ptr );
};


#endif // __HTTP_SERVER_H__

