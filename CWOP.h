#ifndef __CWOP_H__
#define __CWOP_H__
#include "WeatherSink.h"

#define CWOP_PACKET_SIZE (1024)

class CWOP: public WeatherSink
{
public:
   CWOP( Config cfg, WeatherData *wd );
   ~CWOP();

   void newData();
   void main();


private:
   bool createPacket( void );
   int sendToServer( char *fmt, ... );
   int receiveFromServer( char *buffer, size_t size, size_t maxsize );
   bool connectToServer( char *server, size_t serverSize );

private:
   int sock;
   char packet[CWOP_PACKET_SIZE];
   char login[CWOP_PACKET_SIZE];
   std::string latitude;
   std::string longitude;
   pthread_mutex_t notify;
   std::vector<std::string> servers;
   std::vector<std::string>::iterator currentServer;
};




#endif //__CWOP_H__
