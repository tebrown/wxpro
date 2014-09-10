#include "Hysteresis.h"
#include <time.h>

// Thanks Woz:
// http://publib.boulder.ibm.com/infocenter/macxhelp/v6v81/index.jsp?topic=/com.ibm.vacpp6m.doc/language/ref/clrc15cplr385.htm

using namespace std;

Hysteresis::Hysteresis( int history ):
   maxHistory( history ),
   dbg("Hysteresis")
{
   if ( maxHistory < 60 && maxHistory > 1 )
   {
      dbg.printf(CRIT, "Unreasonably low hysteresis.  Setting to 60 seconds");
      maxHistory = 60;
   }
}

Hysteresis::~Hysteresis()
{
}

const double &Hysteresis::operator=( const double &val )
{
   if ( maxHistory <= 1 )
   {
      newVal = val;
      return val;
   }
   time_t t = time(NULL);
   double sum = 0;
   history[t] = val;
   
   for ( map< time_t, double >::iterator iter = history.begin();
         iter != history.end();  )
   {
      if ( iter->first < t - maxHistory )
      {
         history.erase(iter++);
      }
      else
      {
         sum += iter->second;
         ++iter;
      }
   }

   newVal = sum / history.size();

   dbg.printf(DEBUG, "sum = %f  size = %d  newVal = %f\n", sum, history.size(), newVal );
   return newVal;
}

void Hysteresis::setHistorySize( int history, const char *name )
{
   if ( name != NULL )
   {
      dbg.setName( name );
   }
   if ( maxHistory < 60 && maxHistory > 1 )
   {
      dbg.printf(CRIT, "Unreasonably low hysteresis.  Setting to 60 seconds");
      maxHistory = 60;
   }
   else
   {
      maxHistory = history;
   }
}

Hysteresis::operator float()
{
   return newVal;
}

Hysteresis::operator double()
{
   return newVal;
}

Hysteresis::operator int()
{
   return (int)newVal;
}
