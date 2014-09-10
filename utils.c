#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "Debug.h"


size_t readTimeout( int fd, void *buf, size_t count, time_t timeout )
{
   Debug dbg("readTimeout");
   fd_set          rfds;
   struct timeval  tv;
   int             rval; 
   unsigned char   *ptr = (unsigned char*)buf;
   size_t          readCount = count; // How many bytes left to read

   if ( fd == -1 )
   {
    errno = EBADR;
    return (size_t)-1;
   }

   FD_ZERO( &rfds );
   FD_SET( fd, &rfds );
   tv.tv_sec = timeout;
   tv.tv_usec = 0;

   while ( readCount )
   {
      rval = select( fd + 1, &rfds, NULL, NULL, &tv );

      if ( tv.tv_sec == 0 && tv.tv_usec == 0 )
      {
         errno = ETIMEDOUT;
         return (size_t)-1;
      }

      if ( rval == -1 )
      {
      //    perror("select()");
      }
      else if ( rval && FD_ISSET( fd, &rfds ))
      {
         int r = read( fd, ptr, readCount );
         if ( r > 0 )
         {
             readCount-=r;
             ptr+=r;
         }
         if ( r == 0 )
         {
            return count;
         }
      }
      else
      {
         errno = ETIMEDOUT;
         return (size_t)-1;
      }
   }
   return count;
}   

int strnfcat( char *str, size_t size, const char *format, ... )
{
   va_list args;
   char buf[size];
   va_start( args, format );
   vsnprintf( buf, size, format, args );
   strncat( str, buf, size - strlen( str ) );
   va_end( args );

   return strlen( str );

}

double getJulianDay( int day, int month, int year, int hour, int minute, int second )
{
//   printf("%s (%d, %d, %d, %d, %d, %d)\n", __func__, day, month, year, hour,
 //        minute, second );
   year += 1900;
   float D = day + (hour/24.0) + (minute / (24.0 * 60 )) + ( second / ( 24.0 * 60 * 60 ));
   if ( month <= 2 )
   {
      year -= 1;
      month += 12;
   }
   int A = int( year/100.0 );
   int B = 2 - A + int( A/4.0 );
   int a = int( 365.25 * ( year + 4716 ));
   int b = int( 30.6001* ( month + 1 ));
  // printf("A = %d\n",A );
  // printf("B = %d\n",B );
  //  printf("a = %d\n",a );
  // printf("b = %d\n",b );
   return a + ( b + D + B - 1524.5 );

}

double dms2deg( int hour, int min, float sec )
{
   return ( hour + min/60.0 + sec/3600.0 );
   
   //rval = rval / 
}

std::string deg2dms( double decimalDegrees )
{
   char buf[32];
   bool negative = false;
   if ( decimalDegrees < 0 )
      negative = true;
   decimalDegrees = fabs( decimalDegrees );
  // decimalDegrees /= 15.0;

   int degrees = (int)(decimalDegrees);
   double dmin = ((((decimalDegrees - degrees) * 100)*60.0)/100.0);
   int minutes = (int)dmin;

   double seconds= ((((dmin - minutes) * 100)*60.0)/100.0);

   snprintf(buf, 31, "%c%3d %02d' %.3f\"", negative?'-':' ', degrees, minutes, seconds );
   return buf;
}

double rangeDegrees( double deg )
{
   while ( deg < 0 )
   {
      deg+= 360;
   }
   while (deg > 360 )
   {
      deg-=360;
   }
   return deg;
}

