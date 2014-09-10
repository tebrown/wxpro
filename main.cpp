#include <unistd.h>
#include <signal.h>
#include "VantagePro.h"
#include "testWriter.h"
#include "testReader.h"
#include "wunderground.h"
#include "weatherbug.h"
#include "pwsweather.h"
#include "WeatherSink.h"
#include "WeatherSource.h"
#include "CWOP.h"
#include "Config.h"
#include <vector>
#include "Debug.h"
#include "httpserver.h"
#include "socket.h"
#include "Alarms.h"
#include <string.h>

using namespace std;

int running = true;

void SigHandler( int sigNum )
{
   if ( sigNum == SIGTERM || sigNum == SIGINT )
   {
      running = false;
   }

}

int main( int argc, char **argv )
{
   char *configFile = NULL;
   Debug dbg("main");
   dbg.printf(EMERG, "WXPro DR1\n");
   dbg.printf(EMERG, "Copyright (C) 2008 Travis E. Brown.  All right reserved.\n");
   dbg.printf(EMERG, "Initializing....\n");

   dbg.printf(EMERG, "\n\n");
   dbg.printf(EMERG, "---------------------------------------\n");
   dbg.printf(EMERG, "WxPro DR1 Initilizing\n");

   int c;
   extern char *optarg;
   int pid;

   if (fork()) return 0;
   pid = fork();

   if ( pid )
   {
      dbg.printf(EMERG, "Backgrounding process. PID = %d\n", pid );
      return 0;
   }



   
   while (( c= getopt( argc, argv, "c:")) != -1 )
   {
      switch (c)
      {
         case 'c':
            configFile = optarg;
            break;
      }
   }

   if ( configFile == NULL )
   {
      configFile = strdup("wxpro.cfg");
   }
   dbg.printf(EMERG, "Reading config file: %s\n", configFile );
   signal( SIGINT, SigHandler );
   signal( SIGUSR1, SigHandler );
   signal( SIGUSR2, SigHandler );
   signal( SIGTERM, SigHandler );
   signal( SIGPIPE, SigHandler );

   Config cfg(configFile);
   if ( cfg.getInteger("test_only") == 1 )
   {
      printf( "Test Only.  No data will be sent to wunderground/CWOP\n");
      dbg.printf(EMERG, "Test Only.  No data will be sent to wunderground/CWOP\n");
   }
   WeatherData wd( cfg );
   vector< WeatherSource * > sources;
   vector< WeatherSink * > sinks;
   if ( cfg.getInteger("test_only") == 1 )
   {
      sources.push_back( new TestReader(cfg ));
   }
   else
   {
      sources.push_back( new VantagePro(cfg ));
   }

   sinks.push_back( new Wunderground(cfg, &wd));
   sinks.push_back( new WeatherBug(cfg, &wd));
   sinks.push_back( new PWSWeather(cfg, &wd));
   //sinks.push_back( new TestWriter(cfg, &wd));
   sinks.push_back( new CWOP(cfg, &wd));
   sinks.push_back( new HTTPServer(cfg, &wd));
   sinks.push_back( new Socket(cfg, &wd));
   sinks.push_back( new Alarms(cfg, &wd));

   bool successfulRead = false;


   while ( running )
   {
      successfulRead = false;
      wd.now->updateTime();
      for ( vector< WeatherSource *>::iterator iter = sources.begin();
            iter!=sources.end();
            ++iter )
      {
         successfulRead |= (*iter)->read( &wd );
      }
      
      if ( successfulRead )
      {
         for ( vector< WeatherSink *>::iterator iter = sinks.begin();
               iter!=sinks.end();
               ++iter )
         {
            (*iter)->newData( );
         }
      }
      else
      {
         dbg.printf(CRIT, "No successful read.  Skipping writes\n");
      }
      sleep(1);
   }


   // Delete all our sources and sinks
   for ( vector< WeatherSource *>::iterator iter = sources.begin();
         iter!=sources.end(); ++iter )
   {
      delete ( *iter );
   }
   sources.erase( sources.begin(), sources.end() );
   for ( vector< WeatherSink *>::iterator iter = sinks.begin();
         iter!=sinks.end(); ++iter )
   {
      delete ( *iter );
   }
   sinks.erase( sinks.begin(), sinks.end() );
   //printf("Cleaning up...\n\r\n");

   return 0;
}
