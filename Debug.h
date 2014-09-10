#ifndef __WX_DEBUG_H__
#define __WX_DEBUG_H__
#include <stdio.h>

#define LOGFILE "/var/log/wxpro.log"

enum DEBUG_LEVEL { EMERG=0, ALERT, CRIT, ERR, WARNING, NOTICE, INFO, DEBUG };
/*
EMERG    system is unusable               
ALERT    action must be taken immediately 
CRIT     critical conditions              
ERR      error conditions                 
WARNING  warning conditions               
NOTICE   normal but significant condition 
INFO     informational                    
DEBUG    debug-level messages             
*/

class Debug
{
public:
   Debug( const char *name );

   void setName( const char *name );

   int printf( DEBUG_LEVEL level, const char * format, ... );

   int logLevel;

   static FILE *fp;

private:
   const char *name;
};

#endif // __WX_DEBUG_H__
