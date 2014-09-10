/**
 * @author Travis E. Brown
 * @version 1.0
 *
 * WeatherData is the data aggregator.  Sources put data into the structure
 * using setValue, and sinks pull it out using getValue.  Sources don't need
 * to set all the properties, and in fact, shouldn't set some of them.  The
 * setValue will complain if this happens.
 */

#ifndef __WEATHER_DATA_H__
#define __WEATHER_DATA_H__

#include "Wind.h"
#include "Rain.h"
#include "Database.h"
#include "Now.h"
#include "Config.h"
#include "Hysteresis.h"
#include <map>
#include <vector>
#include "Barometer.h"
#include <vector>
#include "Almanac.h"

#define NO_VALUE (0xffffffff)

class WeatherData
{
   public:
      enum PROPERTY { insideTemp, insideHumidity, outsideTemp, outsideHumidity,
         windSpeed, windDirection, averageWindDirection, averageWindSpeed, 
         windGust, windGustDir, dailyRain, rainRate, dailyET, UV, 
         solarRadiation, solarPercent, stationPressure, SLP, altimeter, 
         dewPoint, heatIndex, humidex, windChill, apparentTemp, 
         cloudHeight, rain24Hour, instantRain, END };

   public:
      /**
       * Default constructor for Weather Data.
       *
       * @param cfg Configuration item
       */
      WeatherData( Config cfg );

      /**
       * Default constructor for Weather Data.
       */
      ~WeatherData();

      /**
       * Sets a weather property to the specified value.  This should be used
       * by sources to add new data
       *
       * @param property weather property
       * @param value value to set the property to
       * @return bool TRUE on succes, FALSE on failure
       */
      bool setValue( PROPERTY property, double value );


      bool hasData( PROPERTY property );
      double getValue( PROPERTY property, time_t howLongAgo, 
            time_t *actualTime = NULL );

      double getHigh( PROPERTY property, time_t howLongAgo, 
            time_t *actualTime );
      double getLow( PROPERTY property, time_t howLongAgo, time_t 
            *actualTime );
      double getTrend( PROPERTY property );
      bool newData();

      const char *propertyToString( PROPERTY property );


   public: // public data members
      Now *now;

   private: // types
      enum DBTable { HOURLY, DAILY };
      enum MINMAX { MIN, MAX };
      enum SUMMARYTYPE { DAILYSUMMARY = 0, DAYTIMESUMMARY = 1,
         NIGHTTIMESUMMARY =2 };

   private: // helper functions
      
      // Copy data from temporary set structure to actual structure
      void copyData( void );

      double getSolarMax();
      double getDewPoint();
      double getWindChill();
      double getHumidex();
      double getHeatIndex();
      double getApparentTemp();
      double getCloudHeight();

      void updateDB();
      void updateSummary( DBTable table, SUMMARYTYPE type = DAILYSUMMARY );
      void doMaintenance();
      double computeTrend( PROPERTY property, int duration /* hours */ );
      double getCurrent( PROPERTY property );
      double getFromObs( PROPERTY property, time_t howLongAgo, time_t
            *actualTime );
      double getFromHourly( PROPERTY property, time_t howLongAgo, time_t
            *actualTime );
      double getFromDaily( PROPERTY property, time_t howLongAgo, time_t
            *actualTime );
      void computeTrends();
      double refreshSolarMax();
      double getStationPressure();
      double getStationVaporPressure( double );
      double getActualVaporPressure( double, double );
      double humidityCorrection( double, double, double );

      void refreshAlmanacs();

   private: // data members
      Config cfg;
      Database db;
      double currentSolarMax;
      Wind wind;
      Rain rain;
      std::map< PROPERTY, double > setValues;
      std::map< PROPERTY, Hysteresis > inst;
      std::map< PROPERTY, double > trend;
      std::map< PROPERTY, bool > dataAvailable;
      Debug dbg;
      Barometer baro;

      pthread_rwlock_t rwLock;

      Almanac almanacYesterday;
      Almanac almanacToday;
      Almanac almanacTomorrow;

      bool dayTime;
      
};

#endif // __WEATHER_DATA_H__
