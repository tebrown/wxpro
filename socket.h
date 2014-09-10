#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__
#include "WeatherSink.h"

#define MAX_SOCKET_BUF (1024)


class Socket:public WeatherSink
{
public:
   Socket( Config cfg, WeatherData *wd );
   ~Socket();

   void worker( int client );
   void main( void );
   void newData() {};

private:
   int processRequest( FILE *fp );
   std::string getCurrent();
   int outputXMLFile( FILE *outFp, const char *filename );
   void return404( FILE *fp, std::string url );

private:
   char buffer[MAX_SOCKET_BUF];
};

typedef struct 
{
   Socket *server;
   int client;
} SocketThread_t;

extern "C" {
   void *startSocketThread( void * ptr );
};


#endif // __SOCKET_SERVER_H__

