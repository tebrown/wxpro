#ifndef __ALMANAC_H__
#define __ALMANAC_H__
#include <time.h>
#include "Debug.h"
#include "Config.h"
#include <math.h>


class Almanac
{
public:

   Almanac( Config cfg, time_t t = 0, double horizon = -50/60.0 ); 

   void setNewTime( time_t t );
   void setNewTime( struct tm * );

   
   time_t sunrise;
   time_t sunset;
   int lengthOfDay;

// Private variables
private:
   Debug dbg;
   Config cfg;

   float tz;
   struct tm tm;
   double julianDay;
   double lon; // F6
   double lat; // F7
   double horizon; 


private:
   struct dataStruct
   {
      dataStruct( time_t t, int timeCorrection, double horizon, double lat, double lon, bool interpolate = true );

      dataStruct *yesterday;
      dataStruct *tomorrow;

      double julianDay;
      double lat;
      double lon;
      int leapSeconds;  // G4
      long double sunsMeanLongitude; // I13
      long double moonsMeanLongitude; // K13
      long double meanAnomalyOfTheSun; // L13
      long double sunsEquationOfCenter; // M13
      long double sunsTrueLongitude; // N13
      long double moonsMeanAscendingNode; // O13
      long double sunsApparentLongitude; // Q13
      long double T; // G13
      long double meanObliq; // R13
      long double NutationInObliq; // S13
      long double NutationInLong; // T13
      long double TrueObliq; // U13

      struct 
      {
         long double declination; // V
         long double rightAscention; // X
         long double rightAscentionCorrected; // Y
      } sunsApparent;

      struct 
      {
         long double mean;          // AA
         long double apparent;      // AB
      } siderealTimeAtMeridianAt0Hrs;

      long double horizonToNoonAngle; // AD
      long double approxTransitTime; // AF

      struct
      {
         long double n, a, b, c;       // For interpolation
         long double rightAscention;   // AK
         long double siderealTimeAtGreenwich; // AM
         long double hourAngle;     // AO
      } sunsInterpolated;
      /*long double sunsActualRightAscention; // AK
      long double siderealTimeAtGreenwich; // AM
      long double rawHourAngle; // AN*/

      long double approxRise; // AP

      struct
      {
         long double n; // AQ
         long double rightAscention; // AR
         long double siderealTimeAtGreenwich; // AT
         long double a, b, c;       // AH AI AJ
         long double declination;   // AX
         long double hourAngle;     // AY
         long double altitude;      // AZ
      } sunsInterpolatedApparent;

      long double risingCorrection; // BA
      long double approxSet; // BB;

      struct 
      {
         long double rightAscention; // BD;
         long double declination; // BE;
         long double siderealTimeAtGreenwich; // BG
         long double hourAngle; // BH
         long double altitude; // BI
         long double n,a,b,c;  
      } setting;

      long double settingCorrection; // BJ

      /*long double transitTime;
      long double riseTime; 
      long double setTime; */

      time_t sunrise;
      time_t sunset;
      time_t transitTime;
      int lengthOfDay;
      

   };
public:
};

#endif
