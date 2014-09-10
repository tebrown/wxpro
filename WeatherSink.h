#ifndef __WEATHER_SINK_H__
#define __WEATHER_SINK_H__


#include "Config.h"
#include "Debug.h"
#include <pthread.h>
#include <semaphore.h>
#include "WeatherData.h"

class WeatherSink
{
   public:
      WeatherSink( Config cfg, WeatherData *wd,  const char *name );
      virtual ~WeatherSink();

      // This is a function back that is called when new data is available
      // from WeatherData
      virtual void newData( void ) = 0;

      // This function WILL be on it's own thread.  It should probably loop
      // forever.
      virtual void main( ) = 0;

   private:
      pthread_t mainThreadID;
      Now workerTime;

   protected:
      void endMain();
      void startMain();

   protected:
      Config cfg;
      WeatherData *wd;
      sem_t notifyWorker;
      sem_t working;
      const char *name;
      bool threadRunning;
      Debug dbg;
};

extern "C" {
   void *startWeatherSinkMainThread( void *ptr );
}
#endif // __WEATHER_SINK_H__
