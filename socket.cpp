#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "socket.h"
#include "XML.h"
#include <sstream>
#include <iomanip>

using namespace std;

#define MAXBUF (1024)

Socket::Socket( Config cfg, WeatherData *_wd ):
   WeatherSink( cfg, _wd, "Socket Server")
{
   startMain();
}

Socket::~Socket()
{
   endMain();
}

void Socket::return404( FILE *fp, string url )
{
   std::ostringstream oss;
   oss << "<html><head>" << endl;
   oss << "<title>404 Not Found</title>" << endl;
   oss << "</head><body>" << endl;
   oss << "<h1>Not Found</h1>" << endl;
   oss << "<p>The request URL " << url << " was not found on the server.</p>"
      << endl;
   oss << "</body></html>" << endl;

   fprintf(fp, "HTTP/1.1 404 Not Found\n");
   fprintf(fp, "Content-Type: text/html\n");
   fprintf(fp, "Content-Length: %d\n\n", oss.str().size());
   fprintf(fp, "%s", oss.str().c_str());
   fflush(fp);
}

int Socket::outputXMLFile( FILE *outFp, const char *filename )
{
   char buf[ 1024 ];
   struct stat statBuf;

   if ( stat( filename, &statBuf ) == -1 )
   {
      dbg.printf(NOTICE, "error stat'ing %s: %s\n", filename,
            strerror(errno));
      return -1;
   }
   int fileSize = statBuf.st_size;
   int len=0;

   fprintf(outFp, "HTTP/1.1 200 OK\n");
   fprintf(outFp, "Content-type: text/xml\n" );
   fprintf(outFp, "Content-length: %d\n\n", fileSize );

   int inFD = open(filename, O_RDONLY );
   if ( inFD != -1 )
   {
      while( (len = read( inFD, buf, sizeof( buf ))) > 0 )
      {
         fwrite( buf, len, 1, outFp );
      }
   }
   else
   {
      dbg.printf(NOTICE, "Error reading %s: %s\n", filename, strerror( errno
            ));
      return -1;
   }
   close( inFD );
   fflush(outFp);
   return fileSize;
}

string Socket::getCurrent()
{
   char timestamp[ 64 ];
   wd->now->RFC2822( timestamp, 64 );
   XML weather("weather", "version", "1.0");
   XML location("location");
   XML observation("observation");
   observation.addAttribute("timestamp", timestamp );

   {
      XML sensor( "sensor", wd->getValue(WeatherData::outsideTemp, 0 ));
      sensor.addAttribute("type", "outsideTemperature" );
      sensor.addAttribute("units", "&#xb0;F" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::outsideHumidity, 0 ));
      sensor.addAttribute("type", "outsideHumidity" );
      sensor.addAttribute("units", "&#x25;" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::insideTemp, 0 ));
      sensor.addAttribute("type", "insideTemperature" );
      sensor.addAttribute("units", "&#xb0;F" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::insideHumidity, 0 ));
      sensor.addAttribute("type", "insideHumidity" );
      sensor.addAttribute("units", "&#x25;" );
      observation.addNode( sensor );
   }

   {
      double val = wd->getValue(WeatherData::heatIndex, 0 );
      XML sensor( "sensor" );
      if ( val != NO_VALUE )
      {
         sensor.setValue( val );
      }
      sensor.addAttribute("type", "heatIndex" );
      sensor.addAttribute("units", "&#xb0;F" );
      observation.addNode( sensor );
   }

   {
      double val = wd->getValue(WeatherData::humidex, 0 );
      XML sensor( "sensor" );
      if ( val != NO_VALUE )
      {
         sensor.setValue( val );
      }
      sensor.addAttribute("type", "humidex" );
      sensor.addAttribute("units", "&#xb0;F" );
      observation.addNode( sensor );
   }

   {
      double val = wd->getValue(WeatherData::windChill, 0 );
      XML sensor( "sensor" );
      if ( val != NO_VALUE )
      {
         sensor.setValue( val );
      }
      sensor.addAttribute("type", "windChill" );
      sensor.addAttribute("units", "&#xb0;F" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::apparentTemp, 0 ));
      sensor.addAttribute("type", "apparentTemp" );
      sensor.addAttribute("units", "&#xb0;F" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::windDirection, 0 ));
      sensor.addAttribute("type", "windDirection" );
      sensor.addAttribute("units", "&#xb0;" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::windSpeed, 0 ));
      sensor.addAttribute("type", "windSpeed" );
      sensor.addAttribute("units", "mph" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::averageWindDirection, 0 ));
      sensor.addAttribute("type", "averageWindDirection" );
      sensor.addAttribute("units", "&#xb0;" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::averageWindSpeed, 0 ));
      sensor.addAttribute("type", "averageWindSpeed" );
      sensor.addAttribute("units", "mph" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::windGustDir, 0 ));
      sensor.addAttribute("type", "windGustDirection" );
      sensor.addAttribute("units", "&#xb0;" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::windGust, 0 ));
      sensor.addAttribute("type", "windGustSpeed" );
      sensor.addAttribute("units", "mph" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::rainRate, 0 )/100.0);
      sensor.addAttribute("type", "rainRate" );
      sensor.addAttribute("units", "in/hr" );
      observation.addNode( sensor );

   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::instantRain, 0 )/100.0);
      sensor.addAttribute("type", "instantRainRate" );
      sensor.addAttribute("units", "in/hr" );
      observation.addNode( sensor );

   }
   
   {
      XML sensor( "sensor", wd->getValue(WeatherData::dailyRain, 0 )/100.0);
      sensor.addAttribute("type", "dailyRain" );
      sensor.addAttribute("units", "in" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::rain24Hour, 0 )/100.0);
      sensor.addAttribute("type", "rainLast24Hours" );
      sensor.addAttribute("units", "in" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::stationPressure, 0 ));
      sensor.addAttribute("type", "stationPressure" );
      sensor.addAttribute("units", "inHg" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::SLP, 0 ));
      sensor.addAttribute("type", "SLP" );
      sensor.addAttribute("units", "inHg" );
      observation.addNode( sensor );
   }
   
   {
      XML sensor( "sensor", wd->getValue(WeatherData::altimeter, 0 ));
      sensor.addAttribute("type", "altimeter" );
      sensor.addAttribute("units", "inHg" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::dewPoint, 0 ));
      sensor.addAttribute("type", "dewPoint" );
      sensor.addAttribute("units", "&#xb0;F" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::solarRadiation, 0 ));
      sensor.addAttribute("type", "solarRadiation" );
      sensor.addAttribute("units", "W/m^2" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::solarPercent, 0 ));
      sensor.addAttribute("type", "solarRadiationPercent" );
      sensor.addAttribute("units", "&#x25;" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "sensor", wd->getValue(WeatherData::UV, 0 ));
      sensor.addAttribute("type", "UV" );
      sensor.addAttribute("units", "index" );
      observation.addNode( sensor );
   }
   
   {
      XML sensor( "trend" );
      sensor.setValue( wd->getTrend(WeatherData::altimeter ), 3, ios_base::showpos );
      sensor.addAttribute("type", "altimeter" );
      sensor.addAttribute("units", "inHg/hr" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "trend" );
      sensor.setValue(wd->getTrend(WeatherData::outsideTemp ), 3, ios_base::showpos );
      sensor.addAttribute("type", "outsideTemperature" );
      sensor.addAttribute("units", "&#xb0;F/hr" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "trend" );
      sensor.setValue(wd->getTrend(WeatherData::outsideHumidity ), 3, ios_base::showpos );
      sensor.addAttribute("type", "outsideHumiduty" );
      sensor.addAttribute("units", "&#x25;/hr" );
      observation.addNode( sensor );
   }

   {
      XML sensor( "trend" );
      sensor.setValue(wd->getTrend(WeatherData::dewPoint ), 3, ios_base::showpos );
      sensor.addAttribute("type", "dewPoint" );
      sensor.addAttribute("units", "&#xb0;F/hr" );
      observation.addNode( sensor );
   }

   location.addAttribute("postal_code", cfg.getString("postal_code"));
   location.addAttribute("city", cfg.getString("city"));
   location.addAttribute("state", cfg.getString("state"));
   location.addAttribute("latitude", cfg.getDouble("latitude"), 4);
   location.addAttribute("longitude", cfg.getDouble("longitude"), 4);

   location.addNode( observation );
   weather.addNode( location );

   return weather.str( 0, "weather.xsl" );
}

int Socket::processRequest( FILE *fp )
{
   string content;
   content = getCurrent();
   if ( fprintf(fp, "%s", content.c_str() ) == EOF )
      return EOF;
   if ( fflush(fp) == EOF )
      return EOF;
   return 0;

}

void Socket::worker( int client )
{
   int timeout = 5;
   FILE* fp = fdopen(client, "w");
   fd_set wfds;
   FD_ZERO( &wfds );
   FD_SET( client, &wfds );
   struct timeval tv;
   tv.tv_sec = timeout;
   tv.tv_usec = 0;
   
   while ( select( client + 1, NULL, &wfds,  NULL, &tv ) > 0 )
   {
      if ( processRequest( fp ) == EOF )
      {
         return;
      }

      sleep(1);

      tv.tv_sec = timeout;
      tv.tv_usec = 0;
      FD_ZERO( &wfds );
      FD_SET( client, &wfds );
   }
   dbg.printf(NOTICE, "Closing socket connection\n");
   fclose(fp);
   close(client);
}

void Socket::main(void)
{
   struct sockaddr_in addr;
   int sd, addrlen = sizeof(addr);
   fd_set rfds;
   int on = 1;
   pthread_t workerId;

   if ( (sd = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
   {
      dbg.printf(EMERG, "Error creating socket: %s\n", strerror( errno ));
      return;
   }
   if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof( on )) < 0 )
   {
      dbg.printf(EMERG, "setsockopt: %s\n", strerror( errno ));
      return;
   };
   addr.sin_family = AF_INET;
   int port = cfg.getInteger("socket_port");
   if ( port == 0 ) 
      port = 6101;
   dbg.printf(NOTICE, "Listening on port: %d\n", port );
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;
   if ( bind(sd, (struct sockaddr*)&addr, addrlen) < 0 )
   {
      dbg.printf(EMERG, "Error binding: %s\n", strerror( errno ));
      return;
   }
   if ( listen(sd, 20) < 0 )
   {
      dbg.printf(EMERG, "Error listening: %s\n", strerror( errno ));
      return;
   }


   struct timeval tv;
   while ( threadRunning )
   {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      FD_ZERO( &rfds );
      FD_SET( sd, &rfds );

      int rval = select ( sd +1, &rfds, NULL, NULL, &tv );

      if ( rval <= 0 )
      {
         if ( !threadRunning )
         {
            dbg.printf(EMERG, "Leaving\n");
            break;
         }
         else
         {
            continue;
         }
      }
      int client = accept(sd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
      if ( client < 0 )
      {
         dbg.printf(EMERG, "Error accept connection: %s\n", strerror( errno ));
      }
      else
      {
         dbg.printf(NOTICE, "Connected: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
         SocketThread_t *t = new SocketThread_t;
         t->server = this;
         t->client = client;
         pthread_attr_t attr;
         pthread_attr_init( &attr );
         pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
         pthread_create( &workerId, &attr, startSocketThread, (void*)t);
      }
   }
   dbg.printf(DEBUG, "closing socket\n");
   close(sd);
   return;
}

void *startSocketThread( void *ptr )
{
   Debug dbg("Socket Thread");
   SocketThread_t *p =  (SocketThread_t *)ptr;
   Socket *sock = p->server;
   sock->worker( p->client );
   delete( p );
   return NULL;
}

