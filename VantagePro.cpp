#include "VantagePro.h"
#include "utils.h"
#include "units.h"
#include "ccitt.h"
#include <arpa/inet.h> // htons

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include "WeatherSource.h"

VantagePro::VantagePro( Config &cfg ):
    dbg("VantagePro"),
    fd(-1)
{
    VantagePro::pathName = cfg.getString("vantage path").c_str();
    openPath();
}

void VantagePro::openPath()
{
    dbg.printf(CRIT, "(Re)opening %s\n", pathName );
    if ( fd != -1 )
        close( fd );


    fd = open( pathName, O_RDWR | O_NONBLOCK | O_NDELAY);
    if ( fd == -1 )
    {
        dbg.printf(NOTICE, "open(): %s\n", strerror(errno));
        return;
    }
    portConfig();
}


signed long VantagePro::twosComplement( UINT16 val )
{
   long rval = val; 

   dbg.printf(CRIT, "----------\n");
   dbg.printf(CRIT, "val: 0x%04x\n", val);
   if ( val & 1<<15 == 1<<15 )
   {
      dbg.printf(CRIT,"negative!\n");
      rval = -1* ( 0xffff & ~val + 1); 
   }
   dbg.printf(CRIT, "new val: 0x%04x\n", rval );
   return rval;
}

void VantagePro::portConfig()
{
    struct termios  port;

    tcgetattr (fd, &port);

    cfsetispeed (&port, B19200);
    cfsetospeed (&port, B19200);

    /*  ... set port to 8N1
    */
    port.c_cflag &= ~PARENB;
    port.c_cflag &= ~CSTOPB;
    port.c_cflag &= ~CSIZE;
    port.c_cflag |= CS8;
    port.c_cflag |= (CREAD | CLOCAL);
    port.c_cflag |= CRTSCTS;                    /* turn on H/W flow control */

    port.c_iflag &= ~(IXON | IXOFF | IXANY);    /* turn off SW flow control */

    port.c_iflag &= ~(INLCR | ICRNL);           /* turn off other input magic */

    port.c_oflag = 0;                           /* NO output magic wanted */

    port.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    tcsetattr (fd, TCSAFLUSH, &port);

    return;
}

bool VantagePro::wakeup()
{
   int retryCount = 3;
   char buf[2];

   if ( fd == -1 )
   {
      openPath();
   }
   if ( fd == -1 )
      return false;

   while ( retryCount )
   {
      tcflush( fd, TCIOFLUSH );
      write( fd, "\n", 1 ); // send wakeup

      // read back \n\r
      if ( 2 == readTimeout( fd, &buf, 2, 1 ) )
      {
         if ( buf[0] == LF && buf[1] == CR )
             return true;
      }
      sleep(1); // Give the console some time to fix itself
      retryCount--;
   }
   openPath();
   sleep(2);    // Give the console a little more time to fix itself
   return false;
}

bool VantagePro::setClock()
{
   struct tm tm;
   struct timeval tv;
   struct timezone tz;
   tSetTime packet;
   unsigned char ack;

   if ( !wakeup() )
   {
      dbg.printf(CRIT, "%s (%d): Could not wake up console\n", __FUNCTION__, __LINE__);
      return false;
   }

   gettimeofday( &tv, &tz );
   localtime_r( &tv.tv_sec, &tm );
   packet.seconds = tm.tm_sec;
   packet.minutes = tm.tm_min;
   packet.hours = tm.tm_hour;
   packet.day = tm.tm_mday;
   packet.month = tm.tm_mon + 1;
   packet.year = tm.tm_year;
   crc = 0;

   crcAccum( packet.seconds );
   crcAccum( packet.minutes );
   crcAccum( packet.hours );
   crcAccum( packet.day );
   crcAccum( packet.month );
   crcAccum( packet.year );
   packet.CRC = htons( crc );
   write( fd, "SETTIME\n", 8 ); 
   readTimeout( fd, &ack, 1, 2 );

   if ( ack != ACK )
   {
      dbg.printf(CRIT,"%s(%d): ACK not received.  Received 0x%02x \n", __FUNCTION__, __LINE__, ack);
      return false;
   }

   write( fd, (char *)&packet, sizeof(packet) ) ;
   //write( fd, "\n", 1 );

   readTimeout( fd, &ack, 1, 2 );

   if ( ack != ACK )
   {
      dbg.printf(CRIT,"%s(%d): ACK not received.  Received 0x%02x \n", __FUNCTION__, __LINE__, ack);
      return false;
   }

   dbg.printf(NOTICE, "Time on weather station set\n");
   return true;
}

bool VantagePro::read( )
{
   if ( !wakeup() )
   {
      dbg.printf(CRIT, "%s (%d): Could not wake up console\n", __FUNCTION__, __LINE__);
      return false;
   }

   unsigned char  ack;

   write( fd, "LOOP 1\n", 7 ); // send loop command
   readTimeout( fd, &ack, 1, 2 );

   if ( ack != ACK )
   {
      dbg.printf(CRIT,"ACK not received.  Received 0x%02x \n", ack);
      return false;
   }

   int rval = readTimeout( fd, (unsigned char*)&loop, sizeof(loop), 2 );
   crc = 0;
   unsigned char *ch = (unsigned char*)&loop;
   for ( unsigned int i = 0 ; i < sizeof( loop ); i++ )
   {
      crcAccum( *ch++ );
   }
   if ( rval != sizeof(loop) )
   {
      dbg.printf(CRIT,"Read %d bytes, expected %d\n", rval, sizeof(loop));
      return false;
   }

   if ( crc != 0 )
   {
      dbg.printf(CRIT,"CRC error\n");
      return false;
   }


   // This is a kludge because my weather station isn't doing something right,
   // and I don't have a lot of time to look into why I am getting dash values
   // sometimes
   
   if ( loop.barometer == 0x00 )
   {
      dbg.printf(CRIT, "Invalid barometer reading.\n");
      return false;
   }

   if ( loop.insideTemp == 0x7fff )
   { 
      dbg.printf(CRIT, "Invalid inside temperature reading.\n");
      return false;
   }

   if ( loop.outsideTemp == 0x7fff )
   { 
      dbg.printf(CRIT, "Invalid outside temperature reading.\n");
      return false;
   }

   if ( loop.insideHumidity == 0xff )
   {
      dbg.printf(CRIT, "Invalid inside humidity reading.\n");
      return false;
   }

   if ( loop.outsideHumidity == 0xff )
   {
      dbg.printf(CRIT, "Invalid outside humidity reading.\n");
      return false;
   }

   wd->setValue(WeatherData::SLP, loop.barometer / 1000.0);
   wd->setValue(WeatherData::insideTemp, loop.insideTemp / 10.0);

   wd->setValue(WeatherData::insideHumidity, loop.insideHumidity );

   wd->setValue(WeatherData::outsideTemp, loop.outsideTemp / 10.0);
   wd->setValue(WeatherData::outsideHumidity, loop.outsideHumidity );

   wd->setValue(WeatherData::windSpeed, loop.windSpeed );
   wd->setValue(WeatherData::windDirection, loop.windDirection );

   wd->setValue(WeatherData::dailyRain, loop.dayRain );

   wd->setValue(WeatherData::dailyET, loop.dayET );

   wd->setValue(WeatherData::solarRadiation, loop.solarRadiation );
   wd->setValue(WeatherData::UV, loop.UV/10.0 );

   if ( wd->now->isNewMinute() )
   {
      if ( !updateBarometer() )
      {
         return false;
      }
   }

   if ( wd->now->isNewHour() && wd->now->getHour() == 12 )
   {
      if ( !setClock() )
      {
         return false;
      }
   }

   printRecord();
   return true;
}

bool VantagePro::updateBarometer()
{
   dbg.printf(INFO, "Forcing barometer refresh\n");

   if ( !wakeup() )
   {
      dbg.printf(CRIT, "%s (%d): Could not wake up console\n", __FUNCTION__, __LINE__);
      return false;
   }
   write( fd, "BARREAD\n", 8);

   char buf[6];
   readTimeout( fd, &buf, sizeof(buf), 3 );
   if ( buf[2] != 79 && buf[3] != 75 )
   {
      dbg.printf(CRIT, "Could not force barometer update\n");
      return false;
   }

   dbg.printf(DEBUG, "Barometer successfully refreshed\n");
   return true;
}

void VantagePro::crcAccum( unsigned char data )
{
   crc = crc_table [ ( crc >> 8 ) ^ data ] ^ ( crc << 8 );
}

void VantagePro::printRecord()
{
    dbg.printf(DEBUG,"L = %c\n", loop.L );
    dbg.printf(DEBUG,"O = %c\n", loop.O1 );
    dbg.printf(DEBUG,"O = %c\n", loop.O2 );
    dbg.printf(DEBUG,"barTrend = %d\n", loop.barTrend );
    dbg.printf(DEBUG,"packetType = %d\n", loop.packetType );
    dbg.printf(DEBUG,"nextRecord = %d\n", loop.nextRecord );
    dbg.printf(DEBUG,"barometer = %.2f\n", loop.barometer / 1000.0 );
    dbg.printf(DEBUG,"inside temp = %.1f\n", loop.insideTemp / 10.0 );
    dbg.printf(DEBUG,"inside humidity = %d%%\n", loop.insideHumidity );
    dbg.printf(DEBUG,"outside temp = %.1f\n", loop.outsideTemp / 10.0 );
    dbg.printf(DEBUG,"outside humidity = %d%%\n", loop.outsideHumidity );
    dbg.printf(DEBUG,"wind speed = %d\n", loop.windSpeed );
    dbg.printf(DEBUG,"10 min ave wind = %d\n", loop.tenMinAvgWind );
    dbg.printf(DEBUG,"wind direction = %d degrees\n", loop.windDirection );
    dbg.printf(DEBUG,"rain rate = %.2f in/hr\n", loop.rainRate/100.0 );
    dbg.printf(DEBUG,"storm rain = %.2f in\n", loop.stormRain/100.0 );
    dbg.printf(DEBUG,"storm start = xx/xx/xx\n" /* DO SOME MATH HERE */ );
    dbg.printf(DEBUG,"day rain = %.2f\n", loop.dayRain / 100.0 );
    dbg.printf(DEBUG,"month rain = %.2f\n", loop.monthRain / 100.0 );
    dbg.printf(DEBUG,"year rain = %.2f\n", loop.yearRain / 100.0 );
    dbg.printf(DEBUG,"day ET = %.2f\n", loop.dayET / 100.0 );
    dbg.printf(DEBUG,"month ET = %.2f\n", loop.monthET / 100.0 );
    dbg.printf(DEBUG,"year ET = %.2f\n", loop.yearET / 100.0 );
    dbg.printf(DEBUG,"UV = %.1f\n", loop.UV/10.0 );
    dbg.printf(DEBUG,"solar radiation = %d W/m^2\n", loop.solarRadiation );
    dbg.printf(DEBUG,"console voltage = %.2f V\n",  ((loop.consoleBatteryVoltage * 300 )/512.0)/100.0);
    dbg.printf(DEBUG,"forecast icon = %d\n", loop.forecastIcon);
    dbg.printf(DEBUG,"forecast rule = %d\n", loop.forecastRuleNumber );
    dbg.printf(DEBUG,"sunrise = %d\n", loop.sunrise );
    dbg.printf(DEBUG,"sunset = %d\n", loop.sunset );

    // Extra Temp
    // Soil Temp
    // Leaf Temp
    // Extra Humidities
    // soil moisture
    // leaf wetness
    // inside alarms
    // rain alarms
    // outside alarms
    // extra alarms
    // soil and leaf alarms
    // transmitter battery status
    // 
}
