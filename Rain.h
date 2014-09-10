#ifndef __RAIN_H__
#define __RAIN_H__

#include "Now.h"
#include <map>
#include <utility>
#include "Database.h"

#define MAX_RAIN_DURATION ( 24 * ONE_HOUR )

class Rain
{
   typedef std::map< time_t,  int > rainAccumulation_t;


   public:
      Rain( Now *now, Database &_db );
      ~Rain( );

      void update( unsigned int value );

      unsigned int getSum( time_t duration );

      unsigned int getRate( void )
      {
         return getSum( ONE_HOUR );
      }

      double getInstantRainrate()
      {
         return instantRain;   
      }

      void dump();
      void restore();

   private:
      rainAccumulation_t rainAccum;
      int oldDayRain;
      Now *now;
      Debug dbg;
      Database db;
      int instantRain;
      time_t lastRainTime;
};

#endif // __RAIN_H__
