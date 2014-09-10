#include "Now.h"
#include <stdio.h>
#include <string.h>

Now::Now():
   dbg("Now")
{
   updateTime();
   srandom( currentTv.tv_usec );
}

Now::~Now()
{
}

void Now::updateTime()
{
   memcpy( &lastTs, &currentTs, sizeof( struct tm ));
   lastTv.tv_sec = currentTv.tv_sec;
   lastTv.tv_usec = currentTv.tv_usec;

   gettimeofday( &currentTv, &tz );
   localtime_r( &currentTv.tv_sec, &currentTs );
}

time_t Now::unixtime()
{
   return currentTv.tv_sec;
}

size_t Now::RFC2822( char *s, size_t max )
{
   return strftime( s, max, "%a, %d %b %Y %H:%M:%S %z", &currentTs );
}

size_t Now::RFC1123( char *s, size_t max )
{
   return strftime( s, max, "%a, %d %b %Y %T %Z", &currentTs );
}

size_t Now::mySQL( char *s, size_t max )
{
   return strftime( s, max, "%Y-%m-%d %H:%M:%S", &currentTs );
}

float Now::timeSinceLast()
{
   struct timeval tv;
   struct timezone tz;
   gettimeofday( &tv, &tz );

   float rval = (tv.tv_sec - currentTv.tv_sec) + ( tv.tv_usec - currentTv.tv_usec )/1000000.0;
   return rval;
}

bool Now::isNewMinute( )
{
   if ( lastTs.tm_min != currentTs.tm_min )
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool Now::isNewHour( )
{
   if ( lastTs.tm_hour != currentTs.tm_hour )
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool Now::isNewDay( )
{
   if ( lastTs.tm_mday != currentTs.tm_mday )
   {
      return true;
   }
   else
   {
      return false;
   }
}

int Now::getMinute()
{
   return currentTs.tm_min;
}

int Now::getHour()
{
   return currentTs.tm_hour;
}

int Now::getDayOfYear()
{
   return currentTs.tm_yday+1;
}

int Now::isDST()
{
   return currentTs.tm_isdst;
}

double Now::getDecimalTime()
{
   return (currentTs.tm_hour + currentTs.tm_min / 60.0);
}
