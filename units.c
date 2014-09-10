#include "units.h"
#include <math.h>
#include <stdio.h>

// Wind Conversion

// To convert between miles per hour (mph) and knots (kts)
double mph2kts( double mph ) { return mph * 0.8689762; }
double kts2mph( double kts ) { return kts * 1.1507794; }

// To convert between miles per hour (mph) and meters per second (m/s)
double mph2mps( double mph ) { return mph * 0.44704;   }
double mps2mph( double mps ) { return mps * 2.23694;   }

// To convert between miles per hour (mph) and feet per second (ft/s)
double fps2mph( double fps ) { return fps * 0.681818;  }
double mph2fps( double mph ) { return mph * 1.46667;   }

// To convert between miles per hours (mph) and kilometers per hour (km/h): 
double kph2mph( double kph ) { return kph * 0.621371;  }
double mph2kph( double mph ) { return mph * 1.609344;  }

// To convert between knots (kts) and meters per second (m/s)
double kts2mps( double kts ) { return kts * 0.5144444; }
double mps2kts( double mps ) { return mps * 1.9438455; }


// To convert between knots (kts) and feet per second (ft/s)
double fps2kts( double fps ) { return fps * 0.5924838; }
double kts2fps( double kts ) { return kts * 1.6878099; }

// To convert between knots (kts) and kilometers per hour (km/h)
double kph2kts( double kph ) { return kph * 0.5399568; }
double kts2kph( double kts ) { return kts * 1.852;     }

// To convert between meters per second (m/s) and feet per second (ft/s):
double mps2fps( double fps ) { return fps * 0.3048;    }
double fps2mps( double mps ) { return mps * 3.28084;   }

// To convert between meters per second (m/s) and kilometers per hour (km/h):
double kph2mps( double kph ) { return kph * 0.277778;  } 
double mps2kph( double mps ) { return mps * 3.6;       }

// To convert between feet per second (ft/s) and kilometers per hours (km/h)
double fps2kph( double fps ) { return fps * 1.09728;   }
double kph2fps( double kph ) { return kph * 0.911344;  }

// To convert between meters and feet 
double m2ft( double m ) { return m * 3.28084;   }
double ft2m( double ft ) { return ft * 0.3048;    } 

// Temperature Conversions

// To convert between degrees Fahrenheit and degrees Celsius 
double F2C( double F ) { return (5/9.0)*(F-32); }
double C2F( double C ) { return ((9/5.0)*C)+32; }

//  To convert between degrees Fahrenheit and Kelvin
double K2F( double K ) { return (9/5.0)*(K-273.15)+32; }
double F2K( double F ) { return ((5/9.0)*(F-32))+273.15; }

// To convert between degrees Fahrenheit to Rankine 
double R2F( double R ) { return R - 459.69; } 
double F2R( double F ) { return F + 259.69; }

// To convert between degrees Celsius to Kelvin 
double K2C( double K ) { return K - 273.15; }
double C2K( double C ) { return C + 273.15; }

// To convert between degrees Celsius to Rankine
double R2C( double R ) { return (5/9.0) * ( R - 491.69 ); }
double C2R( double C ) { return ((9/5.0) * C ) + 491.69 ; }

// To convert between Kevin and Rankine 
double R2K( double R ) { return (5/9.0)*(R-764.84); }
double K2R( double K ) { return ((9/5.0)*K)+764.84; }

// Rain rate conversions
double in2mm( double in ) { return in * 25.4; }
double mm2in( double mm ) { return mm * 0.0393700787; };

// Pressure
double inHg2hPa( double inHg ) { return 33.8638 * inHg;  }
double hPa2inHg( double hPa )  { return 0.0295300 * hPa; }

// Trig
float deg2rad( float deg ) { return 0.01745329 * deg; }
float rad2deg( float rad ) { return 57.29578 * rad; }

// Time
int jan1local()
{
   return 0;
   //return JAN12000 + (timezone - 3600 * daylight);
}

time_t gmt2local( time_t gmt )
{
   return gmt;
   //return gmt - (timezone - 3600 * daylight);
}

time_t julian2localtime( double dec )
{
   dec += .5; // roll over at midnight instead of noon

   int ip = ((int)dec - 2451545)*60*60*24;
   int secs = (int)((dec-trunc(dec))*(3600*24));
   time_t localtime = ip + jan1local() + secs;
   return gmt2local(localtime);
}

double frac( double num )
{
   double a;
   a = num - floor( num );
   if ( a < 0 ) a+=1;
   return a;
}

std::string DecimalLatitudeToLORAN( double decimalDegrees )
{
   char buf[16];
   char cardinal;

   bool negative = false;
   if ( decimalDegrees < 0 )
      negative = true;
   decimalDegrees = fabs( decimalDegrees );

   int degrees = (int)(decimalDegrees);
   int minutes = (int)((((decimalDegrees - degrees) * 100)*60.0)/100.0);
   int decimalMin = ((int)(((decimalDegrees - degrees) * 100)*60.0) - minutes * 100);

   if ( negative == true )
      cardinal = 'S';
   else
      cardinal = 'N';

   snprintf(buf, 15, "%02d%02d.%02d%c", degrees, minutes, decimalMin, cardinal );
   return buf;
}

std::string DecimalLongitudeToLORAN( double decimalDegrees )
{
   char buf[16];
   char cardinal;

   bool negative = false;
   if ( decimalDegrees < 0 )
      negative = true;
   decimalDegrees = fabs( decimalDegrees );

   int degrees = (int)(decimalDegrees);
   int minutes = (int)((((decimalDegrees - degrees) * 100)*60.0)/100.0);
   int decimalMin = ((int)(((decimalDegrees - degrees) * 100)*60.0) - minutes * 100);

   if ( negative == true )
      cardinal = 'W';
   else
      cardinal = 'E';

   snprintf(buf, 15, "%03d%02d.%02d%c", degrees, minutes, decimalMin, cardinal );
   return buf;
}
