#include "Almanac.h"
#include "math.h"
#include "units.h"
#include "utils.h"
#include <stdio.h>
// http://bodmas.org/astronomy/riset.html#twig01a
//
//
// Test the algorithm from USNO website: 
//    http://aa.usno.navy.mil/data/docs/RS_OneDay.php

Almanac::Almanac( Config _cfg, time_t t, double horizon ):
   dbg("Almanac"),
   cfg(_cfg)
{
   this->lat = cfg.getDouble("latitude");
   this->lon = cfg.getDouble("longitude");
   this->horizon = horizon;
   setNewTime( t );
}

void Almanac::setNewTime( struct tm *tm )
{
   this->tm = *tm;
   this->tm.tm_sec = 0;
   this->tm.tm_hour = 0;
   this->tm.tm_min = 0;
   tz = cfg.getDouble("timezone");
   if ( tm->tm_isdst == 1 )
      tz+=1;


   dbg.printf(DEBUG, "Almanac::setNewTime()\n");

   time_t t = mktime( &this->tm );
   dataStruct alm( t, (int)tz*3600, horizon, lat, lon );
   sunrise = alm.sunrise;
   sunset = alm.sunset;
   lengthOfDay = alm.lengthOfDay;

   
}

Almanac::dataStruct::dataStruct( time_t t, int timeCorrection, double horizon, double lat, double lon, bool interpolate )
{

   struct tm tm;
   sunrise = sunset = transitTime = t;
   localtime_r( &t, &tm );
   julianDay = getJulianDay( tm.tm_mday, 
         tm.tm_mon+1, 
         tm.tm_year, 
         0,
         0,
         0 );


   if ( interpolate )
   {
      yesterday = new dataStruct( t - 86400, timeCorrection, horizon, lat, lon, false);
      tomorrow = new dataStruct( t + 86400, timeCorrection, horizon, lat, lon, false);
   }

   // ( 22.1 )
   long double T = (julianDay - 2451545.0)/36525.0; // Julian Centuries from J2000
   leapSeconds = 64;
   // Compute Sun's Mean Longitude ( 25.2 )
   sunsMeanLongitude = rangeDegrees(280.466450 + 36000.769830*T + 0.0003032 * T * T);

   // Compute Moon's Mean Longitude ( 47.1 )
   moonsMeanLongitude = rangeDegrees(218.3164477 + 481267.88123421 * T - 0.0015786 * T * T + T*T*T/538841 - T*T*T*T/65194000.0);
   // Original: moonsMeanLongitude = rangeDegrees(218.316459 + 481267.8813421 * T - 0.001327 * T * T + T*T*T/538841 - T*T*T*T/65194000.0);

   // Compute the mean anomaly of the sun ( 25.3 )
   meanAnomalyOfTheSun = 357.52911 + 35999.05029*T - .0001537*T*T;
   // Original: meanAnomalyOfTheSun = 357.5291 + 35999.05030*T - .000156*T*T +
      //.00000048 * T * T * T;

   // Compute the sun's equation of center ( 24.4+ )
   sunsEquationOfCenter = (1.914602 - 0.004817 * T - 0.000014*T*T ) * sin( deg2rad(meanAnomalyOfTheSun) );
            //+ ( 0.019993 - 0.000101*T)*sin(deg2rad(2*meanAnomalyOfTheSun)) 
            //+ ( 0.00289)*sin(deg2rad(3*meanAnomalyOfTheSun));
   /* Original: sunsEquationOfCenter = (1.9146 - 0.004817 * T - 0.000014*T*T ) 
            * sin( deg2rad(meanAnomalyOfTheSun) )
            + ( 0.019993 - 0.000101*T)*sin(deg2rad(2*meanAnomalyOfTheSun)) 
            + ( 0.00029)*sin(deg2rad(3*meanAnomalyOfTheSun));*/

   // True longitude of the sun ( 24.4++)
   sunsTrueLongitude = rangeDegrees(sunsMeanLongitude + sunsEquationOfCenter);

   // Moon's Long Mean Ascending Node ( ??? )
   /*moonsMeanAscendingNode = 125.0445479 - 1934.1362891 * T + 0.0020754 * T * T +
      T*T*T/467441.0 - T*T*T*T / 60616000.0;*/
   moonsMeanAscendingNode = 125.9445479 - 1934.1362891 * T + 0.0020754 * T * T +
      T*T*T/467441.0 - T*T*T*T / 60616000.0;

   // 25.5+
   sunsApparentLongitude = rangeDegrees(sunsTrueLongitude - 0.00569 - 0.00478 * sin ( deg2rad( moonsMeanAscendingNode )));

   // p 147
   meanObliq = dms2deg( 23, 26, 21.448 ) - dms2deg( 0, 0, 46.8150 ) * T - dms2deg( 0, 0, 0.00059 ) * ( T * T) + dms2deg( 0, 0, 0.001813 ) * ( T * T * T );
    
   // p 144
   double omega2= (125.04452 - 1934.136261*T);
   double L= (rangeDegrees(280.4665 + 36000.7698*T));
   double LPrime= rangeDegrees(218.3165 + 481267.8813*T);

   long double NutationInObliq = 9.20/3600.0*(cos(deg2rad(omega2))) + 
                              0.57/3600.0*(cos(deg2rad(2*L))) + 
                              0.10/3600.0*(cos(deg2rad(2*LPrime))) - 
                              0.09/3600.0*(cos(deg2rad(2*omega2)));
   long double NutationInLong  =  -17.20/3600.0*(sin(deg2rad(omega2))) - 
                           1.32/3600.0*(sin(deg2rad(2*L))) - 
                           0.23/3600.0*(sin(deg2rad(2*LPrime))) + 
                           0.21/3600.0*(sin(deg2rad(2*omega2)));

   TrueObliq = meanObliq + NutationInObliq;

   // 25.7
   sunsApparent.declination = rad2deg(asin( sin( deg2rad( TrueObliq )) * sin( deg2rad( sunsApparentLongitude ))));

   // 25.7 
   sunsApparent.rightAscention = rangeDegrees(rad2deg( atan( ( cos( deg2rad( TrueObliq )) * tan( deg2rad( sunsApparentLongitude ))))));

   // 25.6
   sunsApparent.rightAscentionCorrected = sunsApparent.rightAscention 
            + 90 * ( trunc( sunsApparentLongitude/90.0) 
            - trunc( sunsApparent.rightAscention/90.0 ));

   // 12.3
   siderealTimeAtMeridianAt0Hrs.mean = rangeDegrees(100.46061837 + 36000.770053608 * T + 0.000387933*T*T - T*T*T/38710000.0);

   // 12.4
   siderealTimeAtMeridianAt0Hrs.apparent = siderealTimeAtMeridianAt0Hrs.mean + NutationInLong * cos(deg2rad( TrueObliq ));

   
   // 15.1
   horizonToNoonAngle = rad2deg(acos((sin(deg2rad( horizon )) 
               - sin( deg2rad( lat ) ) 
               * sin(deg2rad( sunsApparent.declination ))) 
               / ( cos(deg2rad(lat)) * cos(deg2rad(sunsApparent.declination )))));

   // 15.2
   approxTransitTime = (sunsApparent.rightAscentionCorrected + 
         fabs(lon) - siderealTimeAtMeridianAt0Hrs.apparent)/360.0 ;
   if ( approxTransitTime < 0 )
      approxTransitTime += 1;
   if ( approxTransitTime > 1 )
      approxTransitTime -= 1;

   if ( !interpolate )
      return;

   sunsInterpolated.n = approxTransitTime + leapSeconds / 86400.0;
   sunsInterpolated.a = sunsApparent.rightAscentionCorrected - 
      yesterday->sunsApparent.rightAscentionCorrected;
   if ( sunsInterpolated.a < 0 ) sunsInterpolated.a += 360;

   sunsInterpolated.b = tomorrow->sunsApparent.rightAscentionCorrected - 
      sunsApparent.rightAscentionCorrected;
   if ( sunsInterpolated.b < 0 ) sunsInterpolated.b += 360;
   
   sunsInterpolated.c = sunsInterpolated.b-sunsInterpolated.a;
   
   sunsInterpolated.rightAscention = sunsApparent.rightAscentionCorrected +
      (sunsInterpolated.n/2.0)*(sunsInterpolated.a+sunsInterpolated.b+sunsInterpolated.n*sunsInterpolated.c);


   // p 103
   sunsInterpolated.siderealTimeAtGreenwich =
      rangeDegrees(siderealTimeAtMeridianAt0Hrs.apparent + 360.985647 * approxTransitTime);
   // p 103

   sunsInterpolated.hourAngle = sunsInterpolated.siderealTimeAtGreenwich 
      - fabs(lon) - sunsInterpolated.rightAscention;
   float modVal = 180;
   if ( sin(deg2rad(sunsInterpolated.hourAngle)) < 0 )
   {
      modVal = -180;
   }
   sunsInterpolated.hourAngle = fmod( sunsInterpolated.hourAngle, modVal );

   // 15.2
   approxRise = approxTransitTime - horizonToNoonAngle/360.0;

   // Sun's Interpolated Apparent Right Ascention, Declinatipon at Rising Hour
   // Angle and Attitude
   sunsInterpolatedApparent.n = approxRise + leapSeconds / 86400.0;

   sunsInterpolatedApparent.rightAscention = 
      sunsApparent.rightAscentionCorrected 
      + (sunsInterpolatedApparent.n/2.0)
      * (sunsInterpolated.a + sunsInterpolated.b + sunsInterpolated.n * sunsInterpolated.c);
   sunsInterpolatedApparent.siderealTimeAtGreenwich= rangeDegrees(
         siderealTimeAtMeridianAt0Hrs.apparent + 360.985647 * approxRise);
   sunsInterpolatedApparent.a = sunsApparent.declination - yesterday->sunsApparent.declination;
   sunsInterpolatedApparent.b = tomorrow->sunsApparent.declination - sunsApparent.declination;
   sunsInterpolatedApparent.c = sunsInterpolatedApparent.b -
      sunsInterpolatedApparent.a;

   sunsInterpolatedApparent.declination = sunsApparent.declination + ( sunsInterpolatedApparent.n
         / 2.0 )* ( sunsInterpolatedApparent.a + sunsInterpolatedApparent.b +
            sunsInterpolatedApparent.n * sunsInterpolatedApparent.c );
   sunsInterpolatedApparent.hourAngle =
      sunsInterpolatedApparent.siderealTimeAtGreenwich - fabs(lon) -
      sunsInterpolatedApparent.rightAscention;

   // Need formula
   sunsInterpolatedApparent.altitude =
      rad2deg(sin(deg2rad(fabs(lat)))*sin(deg2rad(sunsInterpolatedApparent.declination))+cos(deg2rad(fabs(lat)))*cos(deg2rad(sunsInterpolatedApparent.declination))*cos(deg2rad(sunsInterpolatedApparent.hourAngle)));

   risingCorrection = (sunsInterpolatedApparent.altitude -
         horizon)/(360*cos(deg2rad(sunsInterpolatedApparent.declination))*cos(deg2rad(fabs(lat)))*sin(deg2rad(sunsInterpolatedApparent.hourAngle)));



   approxSet = approxTransitTime + horizonToNoonAngle/360.0;

   setting.n = approxSet + leapSeconds / 86400.0;
   setting.declination = sunsApparent.declination + ( setting.n/2.0 ) * (
         sunsInterpolatedApparent.a
         +sunsInterpolatedApparent.b
         +setting.n*sunsInterpolatedApparent.c );

   setting.a = sunsApparent.rightAscentionCorrected - yesterday->sunsApparent.rightAscentionCorrected;
   if ( setting.a < 0 ) setting.a += 360;

   setting.b = tomorrow->sunsApparent.rightAscentionCorrected - sunsApparent.rightAscentionCorrected;
   if ( setting.b < 0 ) setting.b += 360;
   
   setting.c = setting.b-setting.a;
   setting.rightAscention = sunsApparent.rightAscentionCorrected + ( setting.n
         / 2.0 )*(setting.a+setting.b+setting.n*setting.c);

   setting.siderealTimeAtGreenwich =
      rangeDegrees(siderealTimeAtMeridianAt0Hrs.apparent + 360.985647 * approxSet);
   setting.hourAngle = setting.siderealTimeAtGreenwich - fabs( lon ) -
      setting.rightAscention;

   setting.altitude =
      rad2deg(sin(deg2rad(fabs(lat)))*sin(deg2rad(setting.declination))+cos(deg2rad(fabs(lat)))*cos(deg2rad(setting.declination))*cos(deg2rad(setting.hourAngle )));

   settingCorrection = ( setting.altitude - horizon )/(360*cos(
            deg2rad(setting.declination))*cos(deg2rad(fabs(lat)))*sin(deg2rad(setting.hourAngle)));


   transitTime += (time_t)((approxTransitTime - sunsInterpolated.hourAngle/360.0)*86400) +
      timeCorrection;
   sunrise += (time_t)(( approxRise + risingCorrection ) * 86400) +
      timeCorrection;
   sunset += (time_t)(( approxSet + settingCorrection ) * 86400) +
      timeCorrection;

   lengthOfDay = sunset - sunrise;

return;

   printf("------------\n");
   printf("horizon: %f\n", horizon );
   printf("lat: %f\n", fabs(lat) );
   printf("lon: %f\n", fabs(lon));
   printf("julian day: %f\n", julianDay );
   printf("moon's mean longitude: %.12Lf\n", moonsMeanLongitude );
   printf("sun's Mean Longitude: %Lf\n", sunsMeanLongitude );
   printf("mean anomaly of the sun: %Lf\n", meanAnomalyOfTheSun );
   printf("sun's equation of center: %Lf\n", sunsEquationOfCenter );
   printf("sun's true longitude: %Lf\n", sunsTrueLongitude );
   printf("moon's mean AsceningNode: %Lf\n", moonsMeanAscendingNode );
   printf("sun's apparent longtiude: %Lf\n", sunsApparentLongitude );
   printf("notation in long: %Lf\n", NutationInLong );
   printf("true obliq: %Lf (%s)\n", TrueObliq, deg2dms(TrueObliq).c_str());
   printf("Sun's Apparent Right Ascention And Declination:\n");
   printf("\tDEC: %Lf\n", sunsApparent.declination );
   printf("\tR.A. (raw) %Lf\n", sunsApparent.rightAscention );
   printf("\tR.A. (same quadrant as long): %Lf\n", sunsApparent.rightAscentionCorrected );
   printf("Sidereal Time @ Meridian @ 0 Hrs UT\n");
   printf("\tmean (AA): %Lf\n", siderealTimeAtMeridianAt0Hrs.mean );
   printf("\tapparent (AB): %Lf\n", siderealTimeAtMeridianAt0Hrs.apparent );
   printf("horizon to noon angle: %Lf\n", horizonToNoonAngle );
   printf("approximate transit time (AF): %Lf\n", approxTransitTime );
   //printf("sun's actual right ascention: %Lf\n", sunsRightAscentionCorrected) ;
   printf("Sun's Interpolated Apparent Right Ascention at Transit and Hour Angle:\n");
   printf("\tsidereal time @ Greenwich (AL): %Lf\n", sunsInterpolated.siderealTimeAtGreenwich );
   printf("\thour angle: %Lf\n", sunsInterpolated.hourAngle );
   printf("\tright ascention: %Lf\n", sunsInterpolated.rightAscention );
   printf("\tn = %Lf   a = %Lf   b = %Lf   c = %Lf\n", 
         sunsInterpolated.n,
         sunsInterpolated.a,
         sunsInterpolated.b,
         sunsInterpolated.c);
   printf("approx rise time: %Lf\n", approxRise );
   printf("suns Interpolated Apparent\n");
   printf("\tn: %Lf\n", sunsInterpolatedApparent.n );
   printf("\tR.A: %Lf\n", sunsInterpolatedApparent.rightAscention );
   printf("\tSidereal Time at Greenwich Corrected: %Lf\n",
         sunsInterpolatedApparent.siderealTimeAtGreenwich );
   printf("\tdeclination: %Lf\n", sunsInterpolatedApparent.declination );
   printf("\thour Angle: %Lf\n", sunsInterpolatedApparent.hourAngle );
   printf("\taltitude: %Lf\n", sunsInterpolatedApparent.altitude );
   printf("rising Correction: %Lf\n", risingCorrection );

   printf("approx set: %Lf\n", approxSet );
   printf("setting:\n");
   printf("\tright ascention: %Lf\n", setting.rightAscention );
   printf("\tdeclination: %Lf\n", setting.declination );
   printf("\tsiderealTime@setting: %Lf\n", setting.siderealTimeAtGreenwich); 
   printf("\tsetting hour angle: %Lf\n", setting.hourAngle );
   printf("\tsetting altitude: %Lf\n", setting.altitude);
   printf("\tsetting correction: %Lf\n", settingCorrection );
   //
   //
   printf("\n\n");
   //printf("transitTime: %Lf\n", transitTime );
   //printf("rise Time: %Lf\n", riseTime );
   //printf("set time : %Lf\n", setTime );

//   printf("Y13 = %Lf\n", sunsRightAscentionCorrected );
   //printf("Y12 = %Lf\n", yesterday->sunsRightAscentionCorrected );
}

void Almanac::setNewTime( time_t t )
{
   if ( t == 0 )
   {
      dbg.printf(DEBUG, "setNewTime(): Using current time\n");
      t = time(NULL);
   }
   struct tm tm;
   localtime_r( &t, &tm );
   setNewTime( &tm );
}

/*
void Almanac::setNewTime( int day, int month, int year, int hour, int min,
         int sec )
{
   struct tm tm;
   tm.tm_hour = hour;
   tm.tm_min = min;
   tm.tm_sec = sec;
   tm.tm_mday = day;
   tm.tm_mon = month - 1;
   tm.tm_year = year - 1900;
   setNewTime( &tm );
}*/



