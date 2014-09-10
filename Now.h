#ifndef __NOW_H__
#define __NOW_H__

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "Debug.h"

#define ONE_MINUTE (60)
#define TWO_MINUTES (2 * ONE_MINUTE)
#define TEN_MINUTES (10 * ONE_MINUTE)
#define ONE_HOUR (ONE_MINUTE * 60 )
#define ONE_DAY (ONE_HOUR * 24 )

class Now
{
   public:
      Now();

      ~Now();

      void updateTime();

      time_t unixtime();
      size_t RFC2822( char *s, size_t max );
      size_t RFC1123( char *s, size_t max );
      size_t mySQL( char *s, size_t max );
      struct timeval timeval();
      float timeSinceLast( );

      bool isNewMinute( );
      bool isNewHour( );
      bool isNewDay( );

      int getMinute();
      int getHour();
      int getDayOfYear();
      int isDST();
      double getDecimalTime();

   private:
      struct tm currentTs;
      struct tm lastTs;
      struct timezone tz;
      struct timeval currentTv;
      struct timeval lastTv;
      Debug dbg;
};


#endif // __NOW_H__
