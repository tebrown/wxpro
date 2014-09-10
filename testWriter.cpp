#include "testWriter.h"
#include "time.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Curses info here:
 * http://www.cs.utk.edu/~vose/c-stuff/ncurses.html
 */

int TestWriter::winch = 0;

static void testWriterSigWinchHandler( int sig )
{
   (void) sig;
   TestWriter::winch = 1;
}

void TestWriter::setupWindow()
{
   int h,w;
   mainWin = initscr();
   getmaxyx(stdscr, h, w);
   noecho();
   cbreak();
   nodelay( mainWin, TRUE );
   refresh();
   titleWin = newwin(3,w,0,0);
   dataWin = newwin(h-3,w,3,0);
   getmaxyx( titleWin, h, w );
   box( titleWin, ACS_VLINE, ACS_HLINE );
   box( dataWin, ACS_VLINE, ACS_HLINE );
   wrefresh(titleWin);
   wrefresh(dataWin);
   wrefresh( mainWin );
}

void TestWriter::teardownWindow()
{
   endwin();
}

TestWriter::TestWriter( Config cfg, WeatherData *_wd ):
   WeatherSink( cfg, _wd, "testWriter" )
{
   signal( SIGWINCH, testWriterSigWinchHandler );
   setupWindow();
   pthread_mutex_init( &notify, NULL );
   pthread_mutex_lock( &notify );
   data.cardinal = "";
   data.gustCardinal = "";
   data.instantCardinal = "";
   startMain();
}

TestWriter::~TestWriter()
{
   endMain();
   clear();
   refresh();
   delwin(mainWin);
   delwin(titleWin);
   delwin(dataWin);
   teardownWindow();
   fflush(stdout);
}

void TestWriter::newData()
{
   pthread_mutex_unlock( &notify );
}

void TestWriter::getData()
{
   const char *cardinals[] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW", "N" };
   data.now = wd->now->unixtime();
   data.outsideTemp = wd->getValue( WeatherData::outsideTemp, 0 );
   data.outsideHumidity = wd->getValue( WeatherData::outsideHumidity, 0 );
   data.outsideTempTrend = wd->getTrend( WeatherData::outsideTemp );
   data.outsideHumidityTrend = wd->getTrend( WeatherData::outsideHumidity );
   data.windSpeed = wd->getValue( WeatherData::averageWindSpeed, 0 );
   data.windGust = wd->getValue( WeatherData::windGust, 0 );
   data.windDirection = (int)wd->getValue( WeatherData::averageWindDirection, 0 );

   data.windGustDir = (int)wd->getValue( WeatherData::windGustDir, 0 );

   data.instantWindSpeed = wd->getValue( WeatherData::windSpeed, 0 );
   data.instantWindDirection = (int)wd->getValue( WeatherData::windDirection, 0 );
   data.UV = wd->getValue( WeatherData::UV, 0 );
   data.solarRadiation = wd->getValue( WeatherData::solarRadiation, 0 );
   data.solarRadiationPercent = (int)wd->getValue( WeatherData::solarPercent, 0 );
   data.barometer = wd->getValue( WeatherData::SLP, 0 );
   data.barometricTrend = wd->getTrend( WeatherData::altimeter );
   data.dailyRain = wd->getValue( WeatherData::dailyRain, 0 )/100.0;
   data.rainRate = wd->getValue( WeatherData::rainRate, 0 )/100.0;
   data.rain24Hour = wd->getValue( WeatherData::rain24Hour, 0 )/100.0;
   data.instantRain = wd->getValue( WeatherData::instantRain, 0)/100.0;
   data.dewPoint = wd->getValue( WeatherData::dewPoint, 0 );
   data.apparentTemp = wd->getValue( WeatherData::apparentTemp, 0 );
   data.cloudHeight = (int)wd->getValue( WeatherData::cloudHeight, 0 );
   int i = (int)((data.windDirection+11)/(360/16.0));
   data.cardinal = cardinals[ i ];

   i = (int)((data.windGustDir+11)/(360/16.0));
   data.gustCardinal = cardinals[ i ];

   i = (int)((data.instantWindDirection+11)/(360/16.0));
   data.instantCardinal = cardinals[ i ];

   // Oh, how I hate floating point.  .010 != .099999999999999999
   int trend = (int)(data.barometricTrend * 1000 );
   dbg.printf(DEBUG, "trend: %d\n", trend );
   if ( trend <= -20 )
   {
      data.barometricTrendString = "Falling Rapidly";
   } 
   else if ( trend > -20 && trend <= -10 )
   {
      data.barometricTrendString = "Falling Slowly";
   } 
   else if ( trend > -10 && trend < 10 )
   {
      data.barometricTrendString = "Steady";
   } 
   else if ( trend >= 10 && trend < 20 )
   {
      data.barometricTrendString = "Rising Slowly";
   } 
   else if ( trend >= 20 )
   {
      data.barometricTrendString = "Rising Rapidly";
   } 
   else
   {
      data.barometricTrendString = "Unknown";
   }


}

void TestWriter::main( )
{
   while ( threadRunning )
   {
      if ( pthread_mutex_trylock( &notify ) == 0 )
      {
         getData();
      }
      else
      {
         sleep(1);
      }

      if ( !threadRunning )
      {
         dbg.printf(NOTICE, "Exiting\n");
         return;
      }

      time_t ct = time(NULL);
      char ct_str[30];
      int h,w;
      int row = 0;
      strcpy( ct_str, ctime( &ct ));
      ct_str[strlen(ct_str)-1] = 0;
      if ( winch )
      {
         winch = 0;
         struct winsize size;
         if ( ioctl( fileno( stdout ), TIOCGWINSZ, &size ) == 0 )
         {
            resizeterm( size.ws_row, size.ws_col );
            teardownWindow();
            setupWindow();
         }
      }
      curs_set(0);
      werase( dataWin );
      werase( titleWin );
      box( dataWin, ACS_VLINE, ACS_HLINE );
      box( titleWin, ACS_VLINE, ACS_HLINE );
      getmaxyx( titleWin, h, w );
      if ( cfg.getInteger("test_only") == 1 )
      {
         mvwprintw( titleWin, 1,(w-strlen(ct_str))/2-7,  "(TEST) %s (TEST)", ct_str );
      }
      else
      {
         mvwprintw( titleWin, 1,(w-strlen(ct_str))/2,  "%s", ct_str );
      }
      
      row = 1;
      mvwprintw( dataWin, row++,2,  " Outside Temperature: %.1fF (%+.1fF/hr)", data.outsideTemp, data.outsideTempTrend);
      mvwprintw( dataWin, row++,2,  "    Outside Humidity: %.1f%% (%+.1f%%/hr)", data.outsideHumidity, data.outsideHumidityTrend);
      mvwprintw( dataWin, row++,2,  "           Dew Point: %.1fF", data.dewPoint);
      mvwprintw( dataWin, row++,2,  "       Apparent Temp: %.1fF", data.apparentTemp);
      mvwprintw( dataWin, row++,2,  " Barometric pressure: %.2f\"",  data.barometer );
      mvwprintw( dataWin, row++,2,  "    Barometric trend: %+.03f\"/hr (%s)", data.barometricTrend, data.barometricTrendString.c_str() );
      mvwprintw( dataWin, row++,2,  "        Cloud height: %d ft", data.cloudHeight );

      row++;
      mvwprintw( dataWin, row,2, " Wind Speed (2m avg): %.1f mph", data.windSpeed);
      mvwprintw( dataWin, row++,40, " Instant Wind Speed: %.1f mph", data.instantWindSpeed);
      mvwprintw( dataWin, row,2, "      Wind Direction: %d (%s)", data.windDirection, data.cardinal);
      mvwprintw( dataWin, row++,40, "   Instant Wind Dir: %d (%s)", data.instantWindDirection, data.instantCardinal  );
      mvwprintw( dataWin, row++,2, "           Wind Gust: %.1f mph", data.windGust);
      mvwprintw( dataWin, row++,2, " Wind Gust Direction: %d (%s)", data.windGustDir, data.gustCardinal);
      row++;
      mvwprintw( dataWin, row++,2, "          Daily Rain: %.2f\"      24-hour rain: %.2f\"", data.dailyRain, data.rain24Hour);
      mvwprintw( dataWin, row++,2, "           Rain Rate: %.2f\"/hr   Instant Rain Rate: %.2f\"/hr", data.rainRate, data.instantRain );

      row++;
      mvwprintw( dataWin, row++,2, "     Solar Radiation: %.1f W/m^2", data.solarRadiation);
      mvwprintw( dataWin, row++,2, "       Solar Percent: %d%%", data.solarRadiationPercent);
      mvwprintw( dataWin, row++,2, "                  UV: %.1f", data.UV);
      wrefresh(dataWin);
      wrefresh(titleWin);
      refresh();
   }
   //printf("Outside Temp: %.1f\n", data.outsideTemp );
}
