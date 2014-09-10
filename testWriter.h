#ifndef __TEST_WRITER__
#define __TEST_WRITER__
#include "WeatherSink.h"
#include "ncurses.h"

#define TITLE "CURRENT WEATHER"

class TestWriter: public WeatherSink
{
public:
   TestWriter( Config cfg, WeatherData *wd );
   ~TestWriter();

   void newData();
   void main( );

private:
   struct 
   {
      time_t now;
      double outsideTemp;
      double outsideTempTrend;
      double outsideHumidity;
      double outsideHumidityTrend;
      double solarRadiation;
      int solarRadiationPercent;
      double UV;
      double windSpeed;
      double instantWindSpeed;
      double windGust;
      double barometer;
      double barometricTrend;
      double dailyRain;
      double rainRate;
      double rain24Hour;
      double instantRain;
      double dewPoint;
      int cloudHeight;
      double apparentTemp;
      int windDirection;
      int windGustDir;
      int instantWindDirection;

      const char *cardinal;
      const char *gustCardinal;
      const char *instantCardinal;
      std::string barometricTrendString;
   } data;

   WINDOW *mainWin;
   WINDOW *titleWin;
   WINDOW *dataWin;

   pthread_mutex_t notify;

private:
   void setupWindow();
   void teardownWindow();
   void getData();

public:
   static int winch;
};




#endif //__TEST_WRITER__
