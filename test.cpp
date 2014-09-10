#include <stdlib.h>
#include "utils.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "gethostbyname_r.h"
#include <string.h>
#include "VantagePro.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "XML.h"
#include <iostream>
#include <errno.h>
#include <iomanip>
#include "Database.h"
#include "Now.h"
#include "Almanac.h"
#include "Config.h"
#include <time.h>


int main()
{
   time_t t = time(NULL);
   Config cfg("test.cfg");
   Debug dbg("main");
   Almanac today( cfg, t);

   //printf("%f\n", getJulianDay( 29, 1, 111, 0, 0, 0 ));

   char buf[64];
   struct tm sunriseTm;
   localtime_r( &today.sunrise, &sunriseTm );
   strftime( buf, 64, "%a, %d %b %Y %T %Z", &sunriseTm );
   printf("sunrise: %s\n", buf );

   struct tm sunsetTm;
   localtime_r( &today.sunset, &sunsetTm );
   strftime( buf, 64, "%a, %d %b %Y %T %Z", &sunsetTm );
   printf("sunset: %s\n", buf );

   printf("length of day: %02d:%02d:%02d\n", 
         today.lengthOfDay/3600, 
         (int)((today.lengthOfDay%3600)/60.0),
         (int)((today.lengthOfDay%3600)%60));
   //dbg.printf(EMERG, "end\n");
   //alm.setNewTime( t -86400 );
   //alm.setNewTime( 224769600 );
   //alm.setNewTime( t + 86400  );

   if ( tomorrow.lengthOfDay < today.lengthOfDay  )
   {
      printf( "Tomorrow will be %d seconds shorter than today\n",
            today.lengthOfDay - tomorrow.lengthOfDay );
   } 
   else if ( tomorrow.lengthOfDay > today.lengthOfDay  )
   {
      printf( "Tomorrow will be %d seconds longer than today\n",
            tomorrow.lengthOfDay - today.lengthOfDay );
   }
   else 
   {
      printf( "Tomorrow will be same length as today\n");
   }
   return 0;

}



