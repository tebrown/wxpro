#ifndef __UNITS_H__
#define __UNITS_H__
#include <time.h>
#include <string>

#define JAN12000 (946684800)

// Wind Conversion

// To convert between miles per hour (mph) and knots (kts)
double mph2kts( double mph );
double kts2mph( double kts );

// To convert between miles per hour (mph) and meters per second (m/s)
double mph2mps( double mph );
double mps2mph( double mps );

// To convert between miles per hour (mph) and feet per second (ft/s)
double fps2mph( double fps );
double mph2fps( double mph );

// To convert between miles per hours (mph) and kilometers per hour (km/h): 
double mph2kph( double kph );
double kph2mph( double mph );

// To convert between knots (kts) and meters per second (m/s)
double kts2mps( double kts );
double mps2kts( double mps );


// To convert between knots (kts) and feet per second (ft/s)
double fps2kts( double fps );
double kts2fps( double kts );

// To convert between knots (kts) and kilometers per hour (km/h)
double kph2kts( double kph );
double kts2kph( double kts );

// To convert between meters per second (m/s) and feet per second (ft/s):
double mps2fps( double fps );
double fps2mps( double mps );

// To convert between meters per second (m/s) and kilometers per hour (km/h):
double kph2mps( double kph );
double mps2kph( double mps );

// To convert between feet per second (ft/s) and kilometers per hours (km/h)
double fps2kph( double fps );
double kph2fps( double kph );

// Convert meter to feet
double ft2m( double ft );
double m2ft( double m );

// Temperature Conversions

// To convert between degrees Fahrenheit and degrees Celsius 
double F2C( double F );
double C2F( double C );

//  To convert between degrees Fahrenheit and Kelvin
double K2F( double K );
double F2K( double F );

// To convert between degrees Fahrenheit to Rankine 
double R2F( double R );
double F2R( double F );

// To convert between degrees Celsius to Kelvin 
double K2C( double K );
double C2K( double C );

// To convert between degrees Celsius to Rankine
double R2C( double R );
double C2R( double C );

// To convert between Kevin and Rankine 
double R2K( double R );
double K2R( double K );

// Rain rate conversions
double in2mm( double in );
double mm2in( double mm );

double inHg2hPa( double inHg );
double hPa2inHg( double hPa );


// Trig

float deg2rad( float deg );
float rad2deg( float rad );


// Time
int jan1local();
time_t gmt2local( time_t gmt );
time_t julian2localtime( double dec );


// Math
double frac( double num );

// Geography
std::string DecimalLatitudeToLORAN( double decimalDegrees );
std::string DecimalLongitudeToLORAN( double decimalDegrees );

#endif // __UNITS_H__

