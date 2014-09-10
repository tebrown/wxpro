/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* Download a document and use libtidy to parse the HTML.
 * Written by Jeff Pohlmeyer
 *
 * LibTidy => http://tidy.sourceforge.net
 *
 * gcc -Wall -I/usr/local/include tidycurl.c -lcurl -ltidy -o tidycurl
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <curl/curl.h>

#define URL_BUF_SIZE (1024)

typedef struct 
{
   double outsideTemp;
   int outsideHumidity;
   double dewPoint;
   double barometer;
   double instantWindSpeed;
   int instantWindDirection;
   double avgWindSpeed_2min;
   double windGust_10min;
   double rainRate;
   double dailyRain;
   double lastHourRain;
} WeatherData;

/* curl write callback, to fill tidy's input buffer...  */
uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
  uint r;
  r = size * nmemb;
  tidyBufAppend( out, in, r );
  return(r);
}

/* Traverse the document tree */
int dumpNode(TidyDoc doc, TidyNode tnod, int element, WeatherData *data )
{
  TidyNode child;
  for ( child = tidyGetChild(tnod); child; child = tidyGetNext(child) )
  {
    element++;
    ctmbstr name = tidyNodeGetName( child );
    if ( name )
    {
      /* if it has a name, then it's an HTML tag ... */
      //TidyAttr attr;
      //printf( "%*.*s%s ", indent, indent, "<", name);
      /* walk the attribute list */
      //for ( attr=tidyAttrFirst(child); attr; attr=tidyAttrNext(attr) ) {
        //printf(tidyAttrName(attr));
        //tidyAttrValue(attr)?printf("=\"%s\" ",
                                   //tidyAttrValue(attr)):printf(" ");
      //}
      //printf( ">\n");
    }
    else {
      /* if it doesn't have a name, then it's probably text, cdata, etc... */
      TidyBuffer buf;
      tidyBufInit(&buf);
      tidyNodeGetText(doc, child, &buf);
      //printf("[%d]%s\n", element, buf.bp?(char *)buf.bp:"");
      switch (element)
      {
         case 133:
            sscanf( (char*)buf.bp, "%lf", &(data->outsideTemp) );
            break;
         case 159:
            sscanf( (char*)buf.bp, "%d", &(data->outsideHumidity));
            break;
         case 301:
            sscanf( (char*)buf.bp, "%lf", &(data->dewPoint));
            break;
         case 333:
            sscanf( (char*)buf.bp, "%lf", &(data->barometer));
            break;
         case 391: // wind speed
            if ( sscanf( (char*)buf.bp, "%lf", &(data->instantWindSpeed)) == 0)
            {
               data->instantWindSpeed = 0;
            }
            break;
         case 417: // wind direction
            {
               char b[100];
               int i,j=0;
               for ( i=0; i<strlen((char*)buf.bp); i++)
               {
                  if ( isdigit( ((char*)buf.bp)[i]))
                  {
                     b[j] = ((char*)buf.bp)[i];
                     j++;
                  }
               }
               b[j] = 0;
               sscanf( b, "%d", &(data->instantWindDirection));
            }
            break;
         case 503:
            if ( sscanf( (char*)buf.bp, "%lf", &(data->avgWindSpeed_2min)) == 0 )
            {
               data->avgWindSpeed_2min = 0;
            }
            break;
         case 533:
            if ( sscanf( (char*)buf.bp, "%lf", &(data->windGust_10min)) == 0 )
            {
               data->windGust_10min = 0;
            }
            break;
         case 599:
            sscanf( (char*)buf.bp, "%lf", &(data->rainRate));
            break;
         case 603:
            sscanf( (char*)buf.bp, "%lf", &(data->dailyRain));
            break;
         case 625:
            sscanf( (char*)buf.bp, "%lf", &(data->lastHourRain));
            break;
      }
      tidyBufFree(&buf);
    }
    element++;
    element = dumpNode( doc, child, element, data ); /* recursive */
  }
  return element;
}


int main(int argc, char **argv )
{
   CURL *curl;
   char curl_errbuf[CURL_ERROR_SIZE];
   char url[URL_BUF_SIZE];
   char *username;
   TidyDoc tdoc;
   TidyBuffer docbuf = {0};
   TidyBuffer tidy_errbuf = {0};
   int err;
   if ( argc == 2) 
   {
      username = argv[1];
   }
   else
   {
      username = "parkernathan";
   }
   WeatherData data;
   snprintf(url, URL_BUF_SIZE, "http://www.weatherlink.com/user/%s/index.php?view=summary&headers=0&type=2", username);
   curl = curl_easy_init();
   curl_easy_setopt(curl, CURLOPT_URL, url);
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
   curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
   curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

   tdoc = tidyCreate();
   tidyOptSetBool(tdoc, TidyForceOutput, yes); /* try harder */
   tidyOptSetInt(tdoc, TidyWrapLen, 4096);
   tidySetErrorBuffer( tdoc, &tidy_errbuf );
   tidyBufInit(&docbuf);

   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
   err=curl_easy_perform(curl);
   if ( !err ) 
   {
      err = tidyParseBuffer(tdoc, &docbuf); /* parse the input */
      if ( err >= 0 ) 
      {
         err = tidyCleanAndRepair(tdoc); /* fix any problems */
         if ( err >= 0 ) 
         {
            dumpNode( tdoc, tidyGetRoot(tdoc), 0, &data ); /* walk the tree */
            //err = tidyRunDiagnostics(tdoc); /* load tidy error buffer */
            //if ( err >= 0 ) 
            //{
               //dumpNode( tdoc, tidyGetRoot(tdoc), 0 ); /* walk the tree */
            //   fprintf(stderr, ">> %s\n", tidy_errbuf.bp); /* show errors */
            //}
         }
      }
   }
   else
   {
      fprintf(stderr, "%s\n", curl_errbuf);
   }
   printf("Outside temp: %f\n", data.outsideTemp );
   printf("Outside humidity: %d\n", data.outsideHumidity );
   printf("Dew Point: %f\n", data.dewPoint );
   printf("Barometer: %f\n", data.barometer );
   printf("Wind speed: %f\n", data.instantWindSpeed );
   printf("Wind direction: %d\n", data.instantWindDirection );
   printf("Average Wind: %f\n", data.avgWindSpeed_2min );
   printf("Wind Gust: %f\n", data.windGust_10min);
   printf("rainRate: %f\n", data.rainRate );
   printf("dailyRain: %f\n", data.dailyRain );
   printf("lastHourRain: %f\n", data.lastHourRain );

   /* clean-up */
   curl_easy_cleanup(curl);
   tidyBufFree(&docbuf);
   //tidyBufFree(&tidy_errbuf);
   tidyRelease(tdoc);
   return(err);


  return(0);
}
