#include "Config.h"

using namespace std;


#include "Config.h"
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <stdlib.h>


std::string Config::trim( std::string str )
{
   unsigned int i = str.length();

   // Erase space at the end 
   for ( ; i != 0; i-- )
   {
      if ( !isspace(str[i]) && isprint(str[i]) )
      {
         i++;
         break;
      }
   }
   str.erase( i, str.length() );


   // Erase space at the beginning 
   for ( i = 0; i != str.length(); i++ )
   {
      if ( !isspace(str[i]) && isprint(str[i]) )
      {
         break;
      }
   }
   str.erase( 0, i );

   return str;

}

Config::Config( const char *configFile ):
   dbg("config")
{
   struct stat statBuf;
   std::string line;

   if ( stat( configFile, &statBuf ) != -1 )
   {
      std::ifstream inFile(configFile);
      std::string key, value, assign;
      if ( inFile.is_open() )
      {
         while ( !inFile.eof() )
         {
            getline( inFile, line );
            std::string::size_type comment = line.find("#", 0 );
            if ( comment != std::string::npos )
            {
               line.erase( comment, line.length() );
            }

            line = trim( line );


            if ( line.empty() )
               continue;

            std::string::size_type  assign = line.find('=');
            if ( assign != std::string::npos )
            {
               key = trim(line.substr(0,assign));
               value = trim(line.substr(assign+1));
               dbg.printf(NOTICE, "%s = '%s'\n", key.c_str(), value.c_str());
         //      printf("line: \"%s\" (%d)\n", line.c_str(), line.length());
               //printf("key = \"%s\"   value=\"%s\"\n", key.c_str(),
                     //value.c_str());
               cfg[key] = value;
            }
         }
          
         inFile.close();
      }
   }
   else
   {
      printf("\nERROR!\n");
      printf("Could not open config file: %s\n", configFile );
      dbg.printf(EMERG, "Error opening config file: %s\n", configFile );
      dbg.printf(EMERG, "Exiting!\n");
      exit(0);
   }

   //
}




string Config::getString( string key )
{
   if ( cfg.find(key) == cfg.end() )
   {
      dbg.printf(EMERG, "Could not find config key: %s\n", key.c_str());
      return "";
   }
   return cfg[key];
}

double Config::getDouble( string key )
{
   if ( cfg.find(key) == cfg.end() )
   {
      dbg.printf(EMERG, "Could not find config key: %s\n", key.c_str());
      return 0;
   }
   return atof(cfg[key].c_str());
}

int Config::getInteger( string key )
{
   if ( cfg.find(key) == cfg.end() )
   {
      dbg.printf(EMERG, "Could not find config key: %s\n", key.c_str());
      return 0;
   }
   return atoi( cfg[key].c_str() );
}
