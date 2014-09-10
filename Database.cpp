#include "Database.h"
#include <stdarg.h>
#include "sql.h"


using namespace std;
Database::Database( const char *_filename ):
   filename( _filename ),
   dbg("Database")
{
   string dbVersion;
   int rc = sqlite3_open( filename, &db );
   if ( rc )
   {
      fprintf(stderr, "Couldn't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(0);
   }

   dbg.printf(NOTICE, "We are operating at DB Version: %s\n", DB_VERSION );

   dbVersion = query("select version from schema;")[0]["version"];
   dbg.printf(NOTICE, "File version is : %s\n", dbVersion.c_str());
   while ( dbVersion != DB_VERSION )
   {
      int ver = atoi( dbVersion.c_str() );
      dbg.printf(WARNING,"Upgrading database schema to version: %d\n", ver + 1 );
      switch ( ver )
      {
         case 0:
            query(SQL_schema);
            query(dailyObsSchema);
            query(hourlySummarySchema);
            query(dailySummarySchema);
            query(windSchema);
            query(rainSchema);
            break;
         case 1:
            query(update1to2);
            break;
         case 2:
            query(update2to3);
            break;
         case 3:
            query(update3to4);
            break;
         case 4:
            query(update4to5);
            break;
         case 5:
            query(update5to6);
            break;
         case 6:
            query(update6to7);
            break;
         case 7:
            query(update7to8);
         case 8:
            query(update8to9);
      }

      dbVersion = query("select version from schema;")[0]["version"];
      if (atoi( dbVersion.c_str() ) == ver )
      {
         dbg.printf(EMERG, "Could not upgrade database to version %d.  ABORTING!\n", ver + 1);
         fprintf(stderr, "Could not upgrade database to version %d.  ABORTING!\n", ver + 1);
         exit(0);
      }
   } 
   
}

dbResult Database::query( const char *format, ... )
{
   dbResult rval;
   int bufSize = 1024;
   char *buf = (char*)malloc( bufSize );
   va_list args;
   int newSize = 0;
   char **resultp;
   int nrow;
   int ncolumn;
   int rc;
   char *zErrMsg = 0;

   va_start( args, format );

   newSize = vsnprintf( buf, bufSize, format, args );
   if ( newSize >= bufSize )
   {
      va_end( args );
      va_start( args, format );
      newSize += 1; // account for the \0
      free( buf );
      buf = (char*)malloc( newSize );
      newSize = vsnprintf( buf, newSize, format, args );
             
   }

   dbg.printf(DEBUG, "query: %s\n", buf );
   rc = sqlite3_get_table( db, buf, &resultp, &nrow, &ncolumn, &zErrMsg );
   if ( rc != SQLITE_OK )
   {
      dbg.printf(CRIT, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
      dbg.printf(CRIT, "SQL error: %s\n", zErrMsg );
      dbg.printf(CRIT, "QUERY: %s\n", buf );
      dbg.printf(CRIT, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
      sqlite3_free( zErrMsg );
   }
   for ( int r = 0; r < nrow; r++ )
   {
      for ( int c = 0; c < ncolumn; c++ )
      {
         char *value = resultp[ncolumn*(r+1)+c];
         if ( value != NULL )
         {
            dbg.printf(DEBUG, "(%d) %s = %s\n", r, resultp[c], resultp[ncolumn*(r+1)+c ]);
            rval[r][resultp[c]] = resultp[ncolumn*(r+1)+c];
         }
      }  
   }
   
   sqlite3_free_table(resultp);

   free( buf );
   va_end( args );
   return rval;
}
