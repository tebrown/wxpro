#include "Wind.h"
#include "units.h"
#include "math.h"

using namespace std;

Wind::Wind( Now *_now, Database &_db ):
   now(_now),
   dbg("Wind"),
   db( _db )
{
   restore();
}

Wind::~Wind()
{
   dump();
}


void Wind::restore()
{
   dbResult result = db.query("select * from windData;");
   for ( dbResult::iterator iter = result.begin(); iter!=result.end(); ++iter )
   {
      time_t t = iter->second.asInteger("time");
      float speed = iter->second.asDouble("speed");
      pair< double, double > pr( iter->second.asDouble("dirSin"),
                                 iter->second.asDouble("dirCos"));
      windSpeed[t] = speed; 
      windDirection[t] = pr;
   }
   dbg.printf(NOTICE, "Restored %d wind values\n", result.size());
}

void Wind::dump()
{
   time_t t;
   float speed;
   float dirSin;
   float dirCos;
   dbg.printf(NOTICE, "Dumping %d wind values to database\n", windSpeed.size());

   db.query("BEGIN TRANSACTION;");
   db.query("DELETE FROM windData;");
   for ( windSpeed_t::iterator iter = windSpeed.begin(); 
         iter != windSpeed.end(); ++iter )
   {
      t = iter->first;
      speed = iter->second;
      dirSin = windDirection[t].first;
      dirCos = windDirection[t].second;
      db.query("insert into windData ( time, speed, dirSin, dirCos ) VALUES ( %d, %f, %f, %f );", (int)t, speed, dirSin, dirCos );
   }
   db.query("COMMIT;");
}




void Wind::update( double speed, double direction )
{
   pair<double, double> pr( sin( deg2rad( direction )), 
                            cos( deg2rad( direction )));
   windDirection[now->unixtime()] = pr ;
   windSpeed[now->unixtime()] = speed;

   for ( windSpeed_t::iterator iter = windSpeed.begin(); 
         iter != windSpeed.end(); )
   {
      if ( iter->first <= now->unixtime() - MAX_WIND_DURATION )
      {
         windSpeed.erase(iter++);
      }
      else
      {
         ++iter;
      }
   }

   for ( windDirection_t::iterator iter = windDirection.begin(); 
         iter != windDirection.end(); )
   {
      if ( iter->first <= now->unixtime() - MAX_WIND_DURATION )
      {
         windDirection.erase(iter++);
      }
      else
      {
         ++iter;
      }
   }
}

int Wind::getAverageDirection( time_t duration )
{
   int rval = 0;
   double sinSum = 0;
   double cosSum = 0;

   for ( windDirection_t::iterator iter = windDirection.begin(); 
         iter != windDirection.end();
         ++iter )
   {
      if ( iter->first > now->unixtime() - duration )
      {
         sinSum += iter->second.first;
         cosSum += iter->second.second;
      }
   }
   rval = (int)rad2deg( atan2( sinSum, cosSum ));
   if ( rval < 0 )
      rval += 360;

   return rval;

}

double Wind::getAverageSpeed( time_t duration )
{
   double rval = 0;
   int count = 0;
   for ( windSpeed_t::iterator iter = windSpeed.begin(); 
         iter != windSpeed.end();
         ++iter )
   {
      if ( iter->first > now->unixtime() - duration )
      {
         rval += iter->second;
         count ++;
      }
   }
   return rval / count;
}

int Wind::getDirectionAtMaxSpeed( time_t duration )
{
   double maxSpeed = -1;
   time_t timeAtMax = 0;
   int rval;
   for ( windSpeed_t::iterator iter = windSpeed.begin(); 
         iter != windSpeed.end();
         ++iter )
   {
      if ( iter->first > now->unixtime() - duration )
      {
         if ( iter->second > maxSpeed )
         {
            maxSpeed = iter->second;
            timeAtMax = iter->first;
         }
      }
   }

   dbg.printf(DEBUG, "timeAtMax: %d\n", timeAtMax );
   dbg.printf(DEBUG, "maxSpeed: %f\n", maxSpeed );
   rval = (int)rad2deg( atan2( windDirection[ timeAtMax ].first, windDirection[ timeAtMax ].second ));
   if ( rval < 0 )
      rval += 360;

   return rval;
}

double Wind::getMaxSpeed( time_t duration )
{
   double rval = -1;
   for ( windSpeed_t::iterator iter = windSpeed.begin(); 
         iter != windSpeed.end();
         ++iter )
   {
      if ( iter->first > now->unixtime() - duration )
      {
         if ( iter->second > rval )
         {
            rval = iter->second;
         }
      }
   }
   return rval;
}
