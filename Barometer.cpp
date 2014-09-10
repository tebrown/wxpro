#include "Barometer.h"
#include "units.h"
#include <math.h>

Barometer::Barometer( double _elevation /* ft */):
   vpAlgorithm( vpBolton ), 
   altimeterAlgorithm( altASOS ), 
   slpAlgorithm( slpManBar ), 

   elevation( _elevation ),
   dbg("Barometer"),
   gravity ( 9.80665 ),
   uGC ( 8.31432 ), 
   moleAir ( 0.0289644 ),
   moleWater ( 0.01801528 ),
   gasConstantAir ( uGC/moleAir ), 
   standardSLP ( 1013.25 ),
   standardSlpInHg ( 29.921 ),
   standardTempK ( 288.15 ),
   earthRadius45 ( 6356.766 ),
   standardLapseRate ( 0.0065 ),
   standardLapseRateFt ( standardLapseRate * 0.3048 ), 
   vpLapseRateUS ( 0.00275 ),
   manBarLapseRate ( 0.0117 )   
{
}

Barometer::~Barometer()
{
}

double Barometer::GeopotentialAltitude()
{
   float elevM = ft2m( elevation );
   return (earthRadius45 * 1000 * elevM) / ((earthRadius45 * 1000) + elevM );
}

double Barometer::StationPressureToAltimeter(
      float stationPressure ) // inHg
{
   float geopEl = 0;
   float k1 = 0, k2 = 0;
   float pressureHPa = inHg2hPa( stationPressure );
   float elevationM = ft2m( elevation );
   switch ( altimeterAlgorithm )
   {
      case altASOS:
         // see ASOS training at http://www.nwstc.noaa.gov
         // see also http://wahiduddin.net/calc/density_altitude.htm
         return pow(pow( stationPressure, 0.1903 ) + ( 1.313e-5 * elevation), 5.255 );
         break;
      case altASOS2:
         geopEl = GeopotentialAltitude();
         k1 = standardLapseRate * gasConstantAir / gravity; // approx. 0.190263
         k2 = 8.41728638E-5; // (standardLapseRate / standardTempK) * (Power(standardSLP,  k1)
         return hPa2inHg(pow(pow( pressureHPa, k1) + (k2 * geopEl), 1/k1));
         break;
      case altMADIS:
         // from MADIS API by NOAA Forecast Systems Lab, see http://madis.noaa.gov/madis_api.html
         k1 = 0.190284; // discrepency with calculated k1 probably because Smithsonian used less precise gas constant and gravity values
         k2 = 8.4184960528E-5; // (standardLapseRate / standardTempK) * (Power(standardSLP, k1)
         return hPa2inHg(pow(pow(pressureHPa - 0.3, k1) + (k2 * elevationM), 1/k1));
         break;

      case altNOAA:
      // see http://www.srh.noaa.gov/elp/wxclc/formulas/altimeterSetting.html
        k1 = 0.190284; // discrepency with k1 probably because Smithsonian used less precise gas constant and gravity values
        k2 = 8.42288069E-5; // (standardLapseRate / 288) * (Power(standardSLP, k1SMT);
        return hPa2inHg((pressureHPa - 0.3) * pow(1 + (k2 * (elevationM / pow(pressureHPa - 0.3, k1))), 1/k1));
        break;
      case altWOB:
         // see http://www.wxqa.com/archive/obsman.pdf
         k1 = standardLapseRate * gasConstantAir / gravity; // approx. 0.190263
         k2 = 1.312603E-5; //(standardLapseRateFt / standardTempK) * Power(standardSlpInHg, k1);
         return pow(pow(stationPressure, k1) + (k2 * elevation), 1/k1);
      case altSMT:
      // see WMO Instruments and Observing Methods Report No.19 at http://www.wmo.int/pages/prog/www/IMOP/publications/IOM-19-Synoptic-AWS.pdf
         k1 = 0.190284; // discrepency with calculated value probably because Smithsonian used less precise gas constant and gravity values
         k2 = 4.30899E-5; // (standardLapseRate / 288) * (Power(standardSlpInHg, k1SMT));
        geopEl = GeopotentialAltitude();
        return (stationPressure - 0.01) * pow(1 + (k2 * (geopEl / pow(stationPressure - 0.01, k1))), 1/k1);

      default:
        dbg.printf(EMERG, "Unknown altimeter type!\n");
   }
   return 0;
}

double Barometer::SLPtoStationPressure( 
      float SLP,           // inHg
      float currentTemp,   // degrees F
      float meanTemp,      // degrees F
      int humidity )       // %
{
   return SLP/PressureReductionRatio( 
         SLP, 
         currentTemp, 
         meanTemp, 
         humidity );
}

double Barometer::SaturationVaporPressure( 
      float currentTemp )  // degreesF
{
   float tempC = F2C(currentTemp);
   // see http://cires.colorado.edu/~voemel/vp.html   
   // comparison of vapor pressure algorithms
   // see (for DavisVP) http://www.exploratorium.edu/weather/dewpoint.html
   switch ( vpAlgorithm )
   {
      case vpDavisVP:
         return 6.112 * exp((17.62 * tempC )/(243.12 + tempC ));
         break;
      case vpBuck: // Buck (1996)
         return 6.1121 * exp((18.678 - (tempC/234.5)) * tempC / ( 257.14 + tempC ));
         break;
      case vpBuck81: // Buck(1981)
         return 6.1121 * exp((17.502 * tempC)/(240.97 + tempC)); 
         break;
      case vpBolton: // Bolton(1980)
         return 6.112 * exp(17.67 * tempC / (tempC + 243.5)); 
         break;
      case vpTetenNWS: // Magnus Teten see www.srh.weather.gov/elp/wxcalc/formulas/vaporPressure.html
         return 6.112 * pow(10,(7.5 * tempC / (tempC + 237.7))); 
         break;
      case vpTetenMurray: // Magnus Teten (Murray 1967)
         return pow(10, (7.5 * tempC / (237.5 + tempC)) + 0.7858); 
         break;
      case vpTeten: // Magnus Teten see www.vivoscuola.it/US/RSIGPP3202/umidita/attivita/relhumONA.htm
         return 6.1078 * pow(10, (7.5 * tempC / (tempC + 237.3))); 
         break;
      default:
         dbg.printf(EMERG, "Unknown vapor pressure algorithm!\n");



   }
   return 0;
}

double Barometer::ActualVaporPressure( 
      float currentTemp,   // degreesF
      int humidity )       // %
{
   return ( humidity * SaturationVaporPressure( currentTemp )) / 100.0;
}

double Barometer::HumidityCorrection(
      float currentTemp,   // degrees F
      int humidity )       // humidity
{
   float vapPress = ActualVaporPressure( currentTemp, humidity );
   return ( vapPress * (( 2.3222e-9 * sqrt( ft2m( elevation ))) + ( 2.225e-5 * ft2m( elevation )) + 0.10743 ));
}

double Barometer::VirtualTempK( 
      float pressureHPa,
      float temp,          // degrees F
      int humidity )
{
   const float epsilon = 1 - (moleWater / moleAir); // 0.37802
   float vapPres = 0;
   // see http://www.univie.ac.at/IMG-Wien/daquamap/Parametergencom.html
   // see also http://www.vivoscuola.it/US/RSIGPP3202/umidita/attiviat/relhumONA.htm
   // see also http://wahiduddin.net/calc/density_altitude.htm
   
   // set buck

   VaporPressureAlgorithms v = vpAlgorithm;
   vpAlgorithm = vpBuck;
   vapPres = ActualVaporPressure(temp, humidity ); 
   vpAlgorithm = v;
   return (F2K(temp)) / (1-(epsilon * (vapPres/pressureHPa)));

}

double Barometer::PressureReductionRatio( 
      float SLP,           // inHg
      float currentTemp,   // degrees F
      float meanTemp,      // degrees F
      int humidity )       // %
{
   float hCorr = 0;
   float pressureHPa = inHg2hPa( SLP );
   float geopElevationM = GeopotentialAltitude();
   switch( slpAlgorithm )
   {
      case slpDavisVP:
         if ( humidity > 0 )
         {
            hCorr = (9/5.0) * HumidityCorrection( currentTemp, humidity );
         }
         return pow( 10, ( elevation / ( 122.894311 * ( meanTemp + 460 + ( elevation * vpLapseRateUS/2 ) + hCorr ))));
         break;
      case slpUnivie:
        return  exp(((gravity/gasConstantAir) * geopElevationM)
           / (VirtualTempK(pressureHPa, meanTemp, humidity) + (geopElevationM * standardLapseRate/2)));
        break;
      case slpManBar:
         // see WMO Instruments and Observing Methods Report No.19 at http://www.wmo.int/pages/prog/www/IMOP/publications/IOM-19-Synoptic-AWS.pdf
         // see WMO Instruments and Observing Methods Report No.19 at http://www.wmo.ch/web/www/IMOP/publications/IOM-19-Synoptic-AWS.pdf
         if (humidity > 0) 
         {
            // set buck
            VaporPressureAlgorithms v = vpAlgorithm;
            vpAlgorithm = vpBuck;
            hCorr = (9/5.0) * HumidityCorrection(currentTemp, humidity );
            vpAlgorithm = v;
            // unset buck
         }
         return exp(geopElevationM * 6.1454E-2 / ( meanTemp + 459.7 + (geopElevationM * manBarLapseRate / 2) + hCorr));

      default:
         dbg.printf(EMERG, "Unknown SLP algorithm!\n");
   }
   return 1;
}
