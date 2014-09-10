#ifndef __BAROMETER_H__
#define __BAROMETER_H__

#include "Debug.h"

class Barometer
{

public:
   enum VaporPressureAlgorithms { 
      vpDavisVP,  // algorithm closely approximates calculation used by Davis 
                  // Vantage Pro weather stations and software
      vpBuck,     // this and the remaining algorithms described at 
                  // http://cires.colorado.edu/~voemel/vp.html
      vpBuck81, 
      vpBolton, 
      vpTetenNWS, 
      vpTetenMurray, 
      vpTeten 
   };

   enum AltimeterAlgorithms { 
      altASOS,    // formula described in the ASOS training docs
      altASOS2,   // metric formula that was likely used to derive the altASOS 
                  // formula
      altMADIS,   // apparently the formula used by the MADIS system
      altNOAA,    // essentially the same as aaSMT with any result differences 
                  // caused by unit conversion rounding error and geometric 
                  // vs. geopotential elevation
      altWOB,     // Weather Observation Handbook (algorithm similar to altASOS 
                  // & altASOS2 - main differences being precision of constants 
                  // used)
      altSMT      // Smithsonian Meteorological Tables (1963)
   };
   enum SLPAlgorithms { 
      slpDavisVP, // algorithm closely approximates SLP calculation used 
                  // inside Davis Vantage Pro weather equipment console 
                  // (http://www.davisnet.com/weather)
      slpUnivie,  // http://www.univie.ac.at/IMG-Wien/daquamap/Parametergencom.html
      slpManBar   // from Manual of Barometry (1963)
   };

   Barometer( double elevation /* ft */);
   ~Barometer();

   double SLPtoStationPressure( 
         float SLP,           // inHg
         float currentTemp,   // degrees F
         float meanTemp,      // degrees F
         int humidity );      // % ( 87 = 87%)

   double StationPressureToAltimeter(
         float stationPressure );// inHg

private:
   double PressureReductionRatio( 
         float SLP,           // inHg
         float currentTemp,   // degrees F
         float meanTemp,      // degrees F
         int humidity );      // %

   double HumidityCorrection(
         float currentTemp,   // degrees F
         int humidity );      // humidity

   double ActualVaporPressure( 
         float currentTemp,   // degreesF
         int humidity );      // %

   double SaturationVaporPressure( 
         float currentTemp ); // degreesF

   double GeopotentialAltitude();

   double VirtualTempK( 
      float pressureHPa,
      float temp,          // degrees F
      int humidity );

public:
   VaporPressureAlgorithms vpAlgorithm;
   AltimeterAlgorithms altimeterAlgorithm;
   SLPAlgorithms slpAlgorithm;

private:
   float elevation;
   Debug dbg;

private: 
   // U.S. Standard Atmosphere (1976) constants
   const float gravity;       // g at sea level at latitude 45.5 degrees in m/sec^2
   const float uGC;           // universal gas constant in J/mole-K
   const float moleAir;       // mean molecular mass of air in kg/mole
   const float moleWater;     // molecular weight of water in kg/mole
   const float gasConstantAir; // (287.053) gas constant for air in J/kgK
   const float standardSLP;     // standard sea level pressure in hPa
   const float standardSlpInHg; // standard sea level pressure in inHg
   const float standardTempK;   // standard sea level temperature in Kelvin
   const float earthRadius45;   // radius of the earth at latitude 45.5 degrees in km
   const float standardLapseRate;   // standard lapse rate (6.5C/1000m i.e. 6.5K/1000m)
  const float standardLapseRateFt; // (0.0019812) standard lapse rate per foot (1.98C/1000ft)
  const float vpLapseRateUS;     // lapse rate used by Davis VantagePro (2.75F/1000ft)
  const float manBarLapseRate; // lapse rate from Manual of Barometry (11.7F/1000m, which = 6.5C/1000m)

};

#endif // __BAROMETER_H__
