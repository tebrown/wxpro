#include "Rain.h"
#include "units.h"
#include "math.h"

using namespace std;

Rain::Rain( Now *_now, Database &_db ):
   oldDayRain(-1),
   now(_now),
   dbg("Rain"),
   db( _db ),
   instantRain(0),
   lastRainTime(0)
{
   restore();
}

Rain::~Rain()
{
   dump();
}


void Rain::restore()
{
   dbResult result = db.query("select * from rainData;");
   for ( dbResult::iterator iter = result.begin(); iter!=result.end(); ++iter )
   {
      time_t t = iter->second.asInteger("time");
      if ( t > now->unixtime() - MAX_RAIN_DURATION )
      {
         unsigned int amount = iter->second.asInteger("amount");
         rainAccum[t] = amount; 
      }
   }
   dbg.printf(NOTICE, "Restored %d rain values (sum = %.2f)\n", result.size(),
         getSum(MAX_RAIN_DURATION)/100.0);
}

void Rain::dump()
{
   time_t t;
   float amount;
   dbg.printf(NOTICE, "Dumping %d rain values to database\n", rainAccum.size());

   db.query("BEGIN TRANSACTION;");
   db.query("DELETE FROM rainData;");
   for ( rainAccumulation_t::iterator iter = rainAccum.begin(); 
         iter != rainAccum.end(); ++iter )
   {
      t = iter->first;
      amount = iter->second;
      db.query("insert into rainData ( time, amount ) VALUES ( %d, %f );", (int)t, amount );
   }
   db.query("COMMIT;");
}




void Rain::update( unsigned int value )
{
   if ( oldDayRain < 0 )
   {
      dbg.printf(CRIT, "initializing oldDayRain to %f\n", value );
      oldDayRain = value;
      return;
   }

   int newRain = value - oldDayRain;
   // Handle rollover midnight case.  
   if ( newRain < 0 )
   {
      dbg.printf(CRIT, "Rain Rollover\n");
      newRain = value;
   }

   // There is no need to fill up the table if we didn't get any data
   if ( newRain != 0 )
   {
      rainAccum[now->unixtime()] = newRain;
   }

   for ( rainAccumulation_t::iterator iter = rainAccum.begin(); 
         iter != rainAccum.end(); )
   {
      if ( iter->first <= now->unixtime() - MAX_RAIN_DURATION )
      {
         rainAccum.erase(iter++);
      }
      else
      {
         ++iter;
      }
   }

   time_t timeSinceLastRain = now->unixtime()-lastRainTime;
   if ( timeSinceLastRain < 60*ONE_MINUTE )
   {
      if ( newRain )
      {
         dbg.printf(DEBUG, "newRain: %d\n", newRain );
         dbg.printf(DEBUG, "timeSinceLastRain: %d\n", timeSinceLastRain );

         // Only compute new amount if we have new rain
         instantRain = (int)(newRain / (timeSinceLastRain/3600.0));  // convert to hours
         dbg.printf(DEBUG, "instantRain: %d\n", instantRain );
      }
      else // If we didn't get new rain, we need to compute the current rain 
           // rate as if the next round will have rain.
      {
         int newInstantRain = (int)(1 / ( timeSinceLastRain/3600.0));
         if ( newInstantRain < instantRain )
         {
            dbg.printf(DEBUG, "Rain rate decay algorithm activated. New rain rate = %d\n", newInstantRain );
            instantRain = newInstantRain;
         }
      }
   }
   else
   {
      instantRain = 0;
   }
   
   if ( instantRain < 0 ) 
      instantRain = 0; 

   if ( newRain )
      lastRainTime = now->unixtime();

   oldDayRain = value;
}

unsigned int Rain::getSum( time_t duration )
{
   unsigned int rval = 0;
   int maxRateAge = now->unixtime() - duration;
   for ( rainAccumulation_t::iterator iter = rainAccum.begin(); 
         iter != rainAccum.end(); ++iter )
   {
      if ( iter->first > maxRateAge )
      {
         rval += iter->second;
      }
   }
   return rval;
}

