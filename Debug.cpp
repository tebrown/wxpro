#include "Debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

FILE *Debug::fp = 0;

void DebugReOpen( int sigNum )
{
   if ( sigNum == SIGHUP )
   {
      fclose(Debug::fp);
      Debug::fp = fopen(LOGFILE, "a+" );
   }
}

void Debug::setName( const char *_name )
{
   name = _name;
}

Debug::Debug( const char *_name):
   name(_name)
{
   logLevel = NOTICE;
   if ( fp == 0 )
   {
      //::printf("Setting signal handler\n");
      signal( SIGHUP, DebugReOpen );   /* set own handler */
      fp = fopen(LOGFILE, "a+" );
      if ( fp == NULL )
      {
          ::printf("Error opening %s: %s", LOGFILE, strerror(errno));
      }
      //fp = stderr;
   }
}

int Debug::printf( DEBUG_LEVEL level, const char *format, ... )
{
   int rval;
   char buf[30];
   char buf2[60];
   if ( level <= logLevel )
   {
     va_list args;
     va_start( args, format );
     struct timeval tv;
     struct timezone tz;
     struct tm tm;
     gettimeofday( &tv, &tz );
     localtime_r(&tv.tv_sec, &tm);
     strftime(buf, 29, "%b %d %H:%M:%S",  &tm);
     snprintf(buf2, 59, "%s.%02d <%d>: %s: ", buf, (int)tv.tv_usec/10000, level, name );
     fprintf(fp, buf2);
     rval = vfprintf( fp, format, args );
     va_end( args );
     fflush(fp);
//     sync();
   }
   return rval;
}
