#ifndef __DATABASE_H__
#define __DATABASE_H__
#include <sqlite3.h>
#include <string>
#include <map>
#include "Debug.h"
#include <stdio.h>
#include <stdlib.h>


class dbRow
{
   public:
      std::string &operator[] ( std::string column ) { return m[column]; };
      double asDouble( std::string column ) { return atof(m[column].c_str()); };
      int asInteger( std::string column ) { return atoi(m[column].c_str()); };
      const char *asCString( std::string column ) { return m[column].c_str(); };

   private:
      std::map< std::string, std::string > m;
};

typedef std::map< int, dbRow > dbResult;
 
class Database
{
   public:
      Database( const char *filename = "/usr/local/var/wxhistory.db" );
      dbResult query( const char *format, ... )
         __attribute__((format(printf,2,3)));

   private:
      sqlite3 *db;
      const char *filename;
      Debug dbg;
};

#endif // __DATABASE_H__
