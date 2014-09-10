#ifndef __VANTAGE_PRO_H__
#define __VANTAGE_PRO_H__

#include "types.h"
#include "Debug.h"
#include "WeatherSource.h"
//#include "Instantaneous.h"

#define CR      0x0D
#define LF      0x0A
#define ACK     0x06
#define NAK     0x21
#define CANCEL  0x18

// Forward declarations

struct Instantaneous;

typedef struct tagForecastIcon
{
    UINT8   rain:1;
    UINT8   cloud:1;
    UINT8   partlyCouldy:1;
    UINT8   sun:1;
    UINT8   snow:1;
    UINT8   reserved:3;
} tForecastIcon;

typedef struct tagSETTIME
{
    UINT8   seconds;
    UINT8   minutes;
    UINT8   hours;
    UINT8   day;
    UINT8   month;
    UINT8   year;
    UINT16  CRC;
} __attribute ((packed)) tSetTime;

typedef struct tagLOOP
{
    UINT8   L;
    UINT8   O1;
    UINT8   O2;
    INT8    barTrend;
    UINT8   packetType;
    UINT16  nextRecord          __attribute__ ((packed));
    UINT16  barometer           __attribute__ ((packed));
    INT16   insideTemp          __attribute__ ((packed));
    UINT8   insideHumidity;
    INT16   outsideTemp         __attribute__ ((packed));
    UINT8   windSpeed;
    UINT8   tenMinAvgWind;
    UINT16  windDirection       __attribute__ ((packed));
    
    // Offset 18
    UINT8   extraTemp[7];
    UINT8   soilTemp[4];
    UINT8   leafTemp[4];

    // Offset 33
    UINT8   outsideHumidity;
    UINT8   extraHumidity[7];
    UINT16  rainRate            __attribute__ ((packed));

    // Offset 43
    UINT8   UV;
    UINT16  solarRadiation      __attribute__ ((packed));
    UINT16  stormRain           __attribute__ ((packed));
    UINT16  stormStartDate      __attribute__ ((packed));
    UINT16  dayRain             __attribute__ ((packed));
    UINT16  monthRain           __attribute__ ((packed));
    UINT16  yearRain            __attribute__ ((packed));
    UINT16  dayET               __attribute__ ((packed));
    UINT16  monthET             __attribute__ ((packed));
    UINT16  yearET              __attribute__ ((packed));

    // Offset 62
    UINT8   soilMoisture[4];
    UINT8   leafWetness[4];
    UINT8   insideAlarms;
    UINT8   rainAlarms;
    UINT16  outsideAlarms       __attribute__ ((packed));

    // Offset 74
    UINT8   extraTempHumidAlarms[8];
    UINT8   soilLeafAlarms[4];

    // Offset 86
    UINT8   transmitterBattery;
    UINT16  consoleBatteryVoltage __attribute__ ((packed));
    UINT8   forecastIcon;
    UINT8   forecastRuleNumber;
    UINT16  sunrise             __attribute__ ((packed));
    UINT16  sunset              __attribute__ ((packed));
    UINT8   linefeed;
    UINT8   carriageReturn;
    UINT16  CRC                 __attribute__ ((packed));
} __attribute ((packed)) LOOP;


class VantagePro:public WeatherSource
{
public:
    VantagePro( Config &cfg );
    
    bool read( );

private: // Helper Functions

    void portConfig();
    
    bool wakeup();
     
    void printRecord();

    void openPath();

    void crcAccum( unsigned char data );

    bool updateBarometer();

    bool setClock();

    signed long twosComplement( UINT16 val );

    Debug dbg;

private: // State Variables

    int fd;
    const char *pathName;

    LOOP loop;

    UINT16 crc;
    int lastMinute;

private:
};

#endif // __VANTAGE_PRO_H__
