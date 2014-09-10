#include "CWOP.h"
#include "units.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netdb.h>      /* for gethostbyname */
#include <errno.h>      /* for perror */
#include <string.h>

// Cygwin doesn't have these.  Why not?
#include "gethostbyname_r.h"

#ifndef MSG_EOR 
#define MSG_EOR (0)
#endif

#ifndef MSG_WAITALL
#define MSG_WAITALL (0)
#endif
// end cygwin hackery

CWOP::CWOP( Config cfg, WeatherData *_wd ):
   WeatherSink( cfg, _wd, "CWOP" )
{
   latitude = DecimalLatitudeToLORAN(cfg.getDouble("latitude"));
   longitude = DecimalLongitudeToLORAN(cfg.getDouble("longitude"));
   snprintf(login, CWOP_PACKET_SIZE, "user %s pass %s ver %s\n\r",
         cfg.getString("cwop_user").c_str(), 
         cfg.getString("cwop_password").c_str(),
         cfg.getString("sw_vers").c_str() );
   pthread_mutex_init( &notify, NULL );
   pthread_mutex_lock( &notify );


   startMain();
}

CWOP::~CWOP()
{
   endMain();
}

bool CWOP::createPacket()
{
   char header[255];
   char buf[255];
   snprintf( header, 40, "%s>APRS,TCPIP*:", cfg.getString("cwop_user").c_str());
   time_t t = time(NULL);
   struct tm gmt;
   char st[32];

   gmtime_r( &t, &gmt );
   strftime( st, 31, "%d%H%M", &gmt );
        
   //  Header, including station ID, time, latitude, and longitude
   snprintf(packet, 64, "%s@%sz%s/%s", header, st, latitude.c_str(),
         longitude.c_str());

   // Wind Direction: _...
   if ( wd->hasData( WeatherData::averageWindDirection ) )
      snprintf(buf, 32, "_%03d", 
            (int)wd->getValue(WeatherData::averageWindDirection, 0 ));
   else
      snprintf(buf, 32, "_..." );
   strcat( packet, buf );
   
   // Wind Speed and Gust: /...g...
   if ( wd->hasData( WeatherData::averageWindSpeed ) )
      snprintf(buf, 32, "/%03dg%03d", 
            (int)wd->getValue(WeatherData::averageWindSpeed, 0 ),
            (int)wd->getValue(WeatherData::windGust, 0 ));
   else
      snprintf(buf, 32, "/...g..." );
   strcat( packet, buf );

   // Outside Temp: t...
   if ( wd->hasData( WeatherData::outsideTemp ) )
      snprintf(buf, 32, "t%03d",
            (int)wd->getValue(WeatherData::outsideTemp, 0 ));
   else
      snprintf(buf, 32, "t..." );
   strcat( packet, buf );

   // Daily Rain: P...
   if ( wd->hasData( WeatherData::dailyRain ) )
   {
      snprintf(buf, 32, "P%03d", 
            (int)(wd->getValue(WeatherData::dailyRain, 0 )));
      strcat( packet, buf );
   }
   
   // Hourly Rain: r...
   if ( wd->hasData( WeatherData::rainRate ) )
   {
      snprintf(buf, 32, "r%03d", 
            (int)(wd->getValue(WeatherData::rainRate, 0 )));
      strcat( packet, buf );
   }

   // 24 hour rain: p...
   if ( wd->hasData( WeatherData::rain24Hour ) )
   {
      snprintf(buf, 32, "p%03d", 
            (int)(wd->getValue(WeatherData::rain24Hour, 0 )));
      strcat( packet, buf );
   }

   // Altimeter: b.....
   if ( wd->hasData( WeatherData::altimeter ) )
   {
      snprintf(buf, 32, "b%05d", 
            (int)(inHg2hPa(wd->getValue(WeatherData::altimeter, 0 )) * 10));
      strcat( packet, buf );
   }

   // Humidity: h..
   if ( wd->hasData( WeatherData::outsideHumidity ) )
   {
      int oh = (int)wd->getValue(WeatherData::outsideHumidity, 0 );
      snprintf(buf, 32, "h%02d", oh == 100 ? 0 : oh );
      strcat( packet, buf );
   }

   // Solar Radiation: L... or l... for >= 1000
   if ( wd->hasData( WeatherData::solarRadiation ) )
   {
      int srad = (int)wd->getValue(WeatherData::solarRadiation, 0 );
      if ( srad >= 1000 )
      {
         snprintf( buf, 32, "l%03d", srad - 1000 );
      }
      else
      {
         snprintf( buf, 32, "L%03d", srad );
      }
      strcat( packet, buf );
   }

   // Trailer.  SW ID
   strcat( packet, ".DsWP\n\r" );
   //strcat( packet, "/WX Report {WXPro}\n\r" );

   return true;
}

int CWOP::sendToServer( char *fmt, ... )
{
   int rval = 0;
   va_list args;
   char message[1024];
   va_start( args, fmt );
   if ( vsnprintf(message, 1024, fmt, args ) > 1024 )
   {
      dbg.printf(ALERT, "Message size to large!\n");
      return false;
   }
   va_end( args );
   rval = send( sock, message, strlen(message), MSG_EOR | MSG_DONTWAIT );
   dbg.printf(DEBUG, "send: %s\n", message);
   return rval;
}

int CWOP::receiveFromServer( char *message, size_t size, size_t max_size )
{
   fd_set rfds;
   FD_ZERO( &rfds );
   FD_SET( sock, &rfds );
   struct timeval tv;
   tv.tv_sec = 5;
   tv.tv_usec = 0;
   bzero( message, max_size );

   if ( select( sock + 1, &rfds, NULL, NULL, &tv ) > 0 )
   {
      recv( sock, message, size, MSG_WAITALL );
      dbg.printf(DEBUG, "receive: %s\n", message );
      return true;
   }
   return false;

}

void CWOP::newData()
{
   pthread_mutex_unlock( &notify );
}

bool CWOP::connectToServer( char *serverName, size_t serverSize )
{
   char errBuf[128];
   int gethostBuf[255];
   struct sockaddr_in cwopServerAddr;
   struct hostent host;
   struct hostent *res;
   int fnord;
   std::vector<std::string>::iterator startingServer = currentServer;

   if (( sock = socket( AF_INET, SOCK_STREAM, 0 )) <= 0 )
   {
      strerror_r( errno, errBuf, 128 );
      dbg.printf(EMERG, "Could not create socket: %s", errBuf );
      return false;
   }

   const char *server = cfg.getString("cwop_server").c_str();
   memset( &cwopServerAddr, 0, sizeof( cwopServerAddr ));

   dbg.printf(DEBUG, "server: %s\n", server );
   if ( ( gethostbyname_r( server,
               &host, (char*)&gethostBuf, sizeof(gethostBuf), &res, &fnord )) != 0 )
   {
      dbg.printf(ERR, "gethostbyname failed: %s\n", hstrerror( fnord ));
      return false;
   }

   dbg.printf(INFO, "CWOP Servers:\n");
   for ( int i = 0; host.h_addr_list[i] != 0; i++ )
   {
      dbg.printf(INFO, "\t%s\n",inet_ntoa(*((struct in_addr *)host.h_addr_list[i])));
   }


   for ( int i = 0; host.h_addr_list[i] != 0; i++ )
   {
      // Set the out IP address
      memcpy( &cwopServerAddr.sin_addr, host.h_addr_list[i], host.h_length );
      cwopServerAddr.sin_family = AF_INET;  
      cwopServerAddr.sin_port   = htons(cfg.getInteger("cwop_port"));
      if ( connect( sock, ( const sockaddr *)&(cwopServerAddr), sizeof( struct sockaddr_in)) < 0 )
      {
         strerror_r( errno, errBuf, 128 );
         dbg.printf(ERR, "could not connect to server: %s\n", inet_ntoa(*((struct in_addr *)host.h_addr_list[i])));
      }
      else
      {
         strncpy( serverName, inet_ntoa(*((struct in_addr *)host.h_addr_list[i])), serverSize);
         return true;
      }
   }
   close( sock );
   return false;
}

void CWOP::main()
{
   char buf[255];
   while( threadRunning )
   {
      pthread_mutex_lock( &notify );
      if ( !threadRunning )
      {
         dbg.printf(NOTICE, "Exiting\n");
         return;
      }

      if ( !wd->now->isNewMinute() || wd->now->getMinute() % 10 != cfg.getInteger("cwop_ul_min"))
      {
         dbg.printf(DEBUG, "Not on minute bounary\n");
         continue;
      }

      if ( cfg.getInteger("test_only") != 1 )
      {
         createPacket();
         int randSleepTime = random() % 10;
         dbg.printf(DEBUG, "Sleeping %d seconds before sending data\n", randSleepTime );
         sleep( randSleepTime );
         char serverName[64];
         if ( !connectToServer( serverName, 64) )
         {
            dbg.printf(NOTICE, "Could not connect to any CWOP server\n");
            continue;
         }

         dbg.printf(DEBUG, "sending data to CWOP (%s)\n", serverName);
         if ( !receiveFromServer( buf, 20, 255 ) )
         {
            dbg.printf(ERR, "Could not read from server\n");
            close(sock);
            continue;
         }
         // Check if this is a real APRS server
         for ( int i = 0; i < 20; i++ )
         {
            if ( isalpha(buf[i]) )
               buf[i] = tolower( buf[i] );
         }

         if ( strstr( buf, "aprs" ) == 0 )
         {
            dbg.printf(ERR, "Expected '*aprs*', received: %s\n", buf );
            close( sock );
            continue;
         }

         if ( sendToServer( login ) != -1 )
         {
            if (!receiveFromServer( buf, 25, 255 ))
            {
               dbg.printf(ERR, "Could not read from server\n");
               close(sock);
               continue;
            }

            sendToServer( packet );
         }
         close( sock );
      }
      else
      {
         dbg.printf(NOTICE, "pretending: CWOP data uploading to %s\n", cfg.getString("cwop_server").c_str());
      }
   }
}
