#include "WeatherSink.h"

WeatherSink::WeatherSink( Config _cfg, WeatherData *_wd, const char *_name ):
   workerTime(),
   cfg( _cfg ),
   wd( _wd ),
   name( _name ),
   threadRunning( true ),
   dbg( _name )
{
   dbg.printf(NOTICE, "Initializing module\n" );
}

void WeatherSink::startMain()
{
   pthread_create( &mainThreadID, NULL, startWeatherSinkMainThread, this );
}

void WeatherSink::endMain()
{
   threadRunning = false;
   this->newData();
   dbg.printf(NOTICE, "Waiting for worker thread to finish\n" );
   pthread_join( mainThreadID, NULL );
   dbg.printf(NOTICE, "Worker thread finished\n" );
}

WeatherSink::~WeatherSink()
{
}

void *startWeatherSinkMainThread( void *ptr )
{
   WeatherSink *ws = ( WeatherSink * )ptr;
   ws->main();
   return NULL;
}

