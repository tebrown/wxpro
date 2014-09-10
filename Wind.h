#ifndef __WIND_H__
#define __WIND_H__

#include "Now.h"
#include <map>
#include <utility>
#include "Database.h"

#define MAX_WIND_DURATION ( 60 * ONE_MINUTE )

class Wind
{
   typedef std::map< time_t, std::pair< double, double> > windDirection_t;
   typedef std::map< time_t,  double > windSpeed_t;

   public:
      Wind( Now *now, Database &_db );
      ~Wind( );

      void update( double speed, double direction );

      double getAverageSpeed( time_t duration );
      int getAverageDirection( time_t duration );
      double getMaxSpeed( time_t duration );
      int getDirectionAtMaxSpeed( time_t duration );

      void dump();
      void restore();

   private:
      windDirection_t windDirection;
      windSpeed_t windSpeed;
      Now *now;
      Debug dbg;
      Database db;
};

#endif // __WIND_H__
