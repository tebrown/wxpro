#!/usr/bin/python

from lxml import etree
from lxml import objectify
import curses, sys, signal, time, datetime
import httplib, threading, socket

class WxProCurses:

   class DataFetcher( threading.Thread ):
      def __init__( self, scr ):
         self.scr = scr 
         self.scr.foo = ""
         threading.Thread.__init__(self)

      def run(self):
         if ( len(sys.argv)>1 ):
            host = sys.argv[1].replace("http://", "", 1 )
         else:
            host = "localhost"

         if ( len(sys.argv)>2 ):
            port = sys.argv[2]
         else:
            port = 6100

         conn = httplib.HTTPConnection(host, port)

         while( 1 ):
            try:
               conn.request("GET", "/")
            except socket.gaierror as ex:
               # This error is because of an invalid configuration
               self.scr.immediateError = "%s:%s: %s" % (host, port, ex)
               break
            except socket.error as ex:
               # This can either be an invalid configuration (host or port) or
               # if the server went away for a bit. This is why we don't break
               # here
               self.scr.xml = ""
               self.scr.fetcherError = "%s:%s: %s" % (host, port, ex)
            except httplib.CannotSendRequest:
               # Retry the connection. We can get here if there was a socket
               # error because the server went away for a bit
               conn.close()
               conn = httplib.HTTPConnection(host, port)
            else:
               rval = conn.getresponse()

               if ( rval.status == 200 ):
                  self.scr.xml = rval.read()
               else:
                  self.scr.xml = ""
                  self.scr.fetcherError = "Server Error: %d: %s" % (rval.status, rval.reason)
            time.sleep(1.5)
      

   def __init__(self, stdscr):

      self.stdscr = stdscr
      self.useCelcius = True
      self.fetcherError = ""
      self.immediateError = ""
      self.xml = ""
      self.noDataCount = 0
      try:
         curses.curs_set(0)
      except Exception, e:
         ex = e

      fetcher = self.DataFetcher(self)
      fetcher.setDaemon( True )
      fetcher.start()

      self._setupScreen()

      titleStr = "Connecting to server..."
      self.titleWin.addstr(1,(self.x-len(titleStr))/2, titleStr)
      self.titleWin.refresh()

      while( 1 ):

         self._updateScreen()

         ch = self.stdscr.getch()
         if ( ch == 113 ):  # 113 = 'q'
            stdscr.clear()
            stdscr.refresh()
            curses.endwin()
            return;
         elif ( ch == curses.KEY_RESIZE ):
            curses.endwin()
            self.stdscr=curses.initscr()
            self._setupScreen()
         elif ( ch == 12 ): # ctrl-L
            curses.endwin()
            self.stdscr=curses.initscr()
            self._setupScreen()

   def _setupScreen(self ):
      (self.y, self.x) = self.stdscr.getmaxyx() 
      self.stdscr.timeout(1000)  # make getch timeout after 1 sec so we can 
                                 # update the screen asynchronously

      # We anything less than 24x80 is pretty useless, so let's make that our
      # minimum size
      if ( self.y < 24 ): 
         self.y = 24
      if ( self.x < 80 ):
         self.x = 80
      self.titleWin = curses.newwin(3,self.x,0,0)
      self.dataWin = curses.newwin(self.y-3,self.x,3,0)
      self.titleWin.border(0)
      self.dataWin.border(0)
      self.stdscr.refresh()
      self.dataWin.refresh()
      self.titleWin.refresh()

   def _updateTitleWin(self):
      # Draw the time in the title window
      currentTimeStr = time.ctime()
      self.titleWin.addstr(1,(self.x-len(currentTimeStr))/2, time.ctime())
      self.titleWin.refresh()

   def _showTemp( self, form, val, label=None ):
      temp = val
      if self.useCelcius:
          units = "C"
          if label:
              temp = temp * 5/9 
          else:
              temp = (val - 32) * 5/9 # For the case when it's F/hour
      else:
          units = "F"

      self.dataWin.addstr( form % temp )
      self.dataWin.addch( curses.ACS_DEGREE )
      self.dataWin.addstr( units )
      if label:
          self.dataWin.addstr( label )


   def _showWindDir( self, direction ):
      cardinals = ( "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW", "N" )

      self.dataWin.addstr( "%d" % (direction) )
      self.dataWin.addch( curses.ACS_DEGREE )
      if direction  > len(cardinals):
          direction = 0
      self.dataWin.addstr( " (%s)" % ( cardinals[(int)((direction+11)/(360.0/16))]))


   def _altimeterTrendToStr( self, val ):
      if ( val <= -.020 ):
         return "Falling Rapidly"
      elif ( val >  -.020 and val <= -.010 ):
         return "Falling Slowly"
      elif ( val > -.010 and val < .010 ):
         return "Steady"
      elif ( val >= .010 and val < .020 ):
         return "Rising Slowly"
      elif ( val >= .020 ):
         return "Rising Rapidly"
      else:
         return "<<< UNKNOWN: %d >>>" % ( val );


   def _updateScreen(self):
      if ( len(self.immediateError) > 0 ):
         self.dataWin.erase()
         self.dataWin.border(0)
         self.dataWin.addstr( 2,3,self.immediateError)
      elif ( len(self.xml)==0 ):
         self.noDataCount+=1
         if ( self.noDataCount>10 ):
            # Throw up an error
            self.dataWin.erase()
            self.dataWin.border(0)
            self.dataWin.addstr( 2,3,"Error No data seen for %d seconds" % (
            self.noDataCount))
            self.dataWin.addstr( 3,3,self.fetcherError)
      else: # We have data!
         self.noDataCount = 0
         wxdata = objectify.fromstring(self.xml)
         sensor = {}
         trend = {}
         for el in wxdata.location.observation.iterchildren(tag='sensor'):
            sensor[el.get("type")] = el
         for el in wxdata.location.observation.iterchildren(tag='trend'):
            trend[el.get("type")] = el
         self.titleWin.erase()
         self.titleWin.border(0)
         self.dataWin.erase()
         self.dataWin.border(0)

         # Title window
         observationTime = time.strftime("%a, %d %b %Y %H:%M %Z", time.localtime(int(wxdata.location.observation.get("secs"))))
         titleStr = "%s, %s - %s " % (wxdata.location.get("city"), wxdata.location.get("state"), observationTime)
         self.titleWin.addstr(1,(self.x-len(titleStr))/2, titleStr)
         self.titleWin.refresh()

         # Outside temperature
         self.dataWin.addstr( 1, 3, "Outside Temperature: " )
         self._showTemp( "%.1f", sensor["outsideTemperature"] )
         self._showTemp( " (%+.1f", trend["outsideTemperature"], "/hr)" )

         # Outside humidity
         self.dataWin.addstr( 2, 3, "   Outside Humidity: " )
         self.dataWin.addstr( "%.1f%%  (%+.1f%%/hr)" % ( sensor["outsideHumidity"], trend["outsideHumidity"] ))
   
         # Dew Point
         self.dataWin.addstr( 3, 3, "          Dew Point: ")
         self._showTemp("%.1f", sensor["dewPoint"] )

         # Apparent Temperature
         self.dataWin.addstr( 4, 3, "      Apparent Temp: ")
         self._showTemp("%.1f", sensor["apparentTemp"] )

         # Barometric Pressure
         self.dataWin.addstr( 5, 3, "Barometric Pressure: ")
         self.dataWin.addstr( "%.2f\"" % ( sensor["SLP"] ))

         # Barometric trend
         self.dataWin.addstr( 6, 3, "   Barometric Trend: ")
         self.dataWin.addstr( "%+.03f\"/hr (%s)" % ( trend["altimeter"],
         self._altimeterTrendToStr(trend["altimeter"])))

         # Cloud height
         self.dataWin.addstr( 7, 3, "       Cloud height: ")
         self.dataWin.addstr("%d ft" % ( sensor["cloudHeight"]))

         ### Wind 
         # Average Wind Speed
         self.dataWin.addstr( 9, 3, "Wind Speed (2m avg): ")
         self.dataWin.addstr( "%.1f mph" % (sensor["averageWindSpeed"]))

         # Average Wind Direction
         self.dataWin.addstr( 10, 3, "     Wind Direction: ")
         self._showWindDir(sensor["averageWindDirection"])

         # Wind Gust
         self.dataWin.addstr( 11, 3, "          Wind Gust: ")
         self.dataWin.addstr( "%.1f mph" % (sensor["windGustSpeed"]))

         # Wind Gust Direction
         self.dataWin.addstr( 12, 3, "Wind Gust Direction: ")
         self._showWindDir(sensor["windGustDirection"])

         # Instant Wind Speed
         self.dataWin.addstr( 9, 40, " Instant Wind Speed: ")
         self.dataWin.addstr( "%.1f mph" % (sensor["instantWindSpeed"]))

         # Instant Wind Direction
         self.dataWin.addstr( 10, 40, "   Instant Wind Dir: ")
         self._showWindDir(sensor["instantWindDirection"])

         ### Rain
         # Daily rain and 24-hour rain
         self.dataWin.addstr( 14, 3, "         Daily Rain: %.2f\"                  24-hour rain: %.2f\"" % ( sensor["dailyRain"] , sensor["rainLast24Hours"]))

         # Rain Rate and Instant Rain Rate
         self.dataWin.addstr( 15, 3, "          Rain Rate: %.2f\"             Instant Rain Rate: %.2f\"/hr" % ( sensor["rainRate"], sensor["instantRainRate"] ))

         ### Solar and UV
         # Solar Radiation
         self.dataWin.addstr( 17, 3, "    Solar Radiation: ")
         self.dataWin.addstr( "%.1f W/m^2" % (sensor["solarRadiation"]))

         # Solar Percent
         self.dataWin.addstr( 18, 3, "      Solar Percent: ")
         self.dataWin.addstr( "%d%%" % ( sensor["solarRadiationPercent"] ))

         # UV
         self.dataWin.addstr( 19, 3, "                 UV: ")
         self.dataWin.addstr( "%.1f" % ( sensor["UV"]))
      self.dataWin.refresh()

def main( stdscr ):
   try: 
      WxProCurses( stdscr )
   except KeyboardInterrupt:
      stdscr.clear()
      stdscr.refresh()
      curses.endwin()

curses.wrapper(main)


