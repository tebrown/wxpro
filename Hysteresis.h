#ifndef __HYSTERESIS_H__
#define __HYSTERESIS_H__

#include <map>
#include "Debug.h"
#include <time.h>

class Hysteresis
{
public:
   Hysteresis( int history = 0);
   ~Hysteresis();
   const double &operator =( const double &val );

   void setHistorySize( int history, const char *name = NULL );

   operator float();
   operator int();
   operator double();

private:
   std::map< time_t, double > history;
   int maxHistory;
   double newVal;
   Debug dbg;

};


#endif // __HYSTERESIS_H__
