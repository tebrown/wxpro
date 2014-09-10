#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <map>
#include <string>
#include "Debug.h"


class Config
{
   public:
      Config( const char *configFile );

      double getDouble( std::string );
      std::string getString( std::string );
      int getInteger( std::string );

   private:
      std::string trim( std::string str );

   private:
      std::map< std::string, std::string > cfg;
      Debug dbg;
};

#endif // __CONFIG_H__
