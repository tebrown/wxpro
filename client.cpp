#include "units.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netdb.h>      /* for gethostbyname */
#include <errno.h>      /* for perror */
#include "types.h"
#include "packet.h"

// Cygwin doesn't have these.  Why not?
#include "gethostbyname_r.h"
#ifndef MSG_EOR 
#define MSG_EOR (0)
#endif

int main()
{
      char errBuf[128];
      int gethostBuf[255];
      struct sockaddr_in cwopServerAddr;
      struct hostent host;
      struct hostent *res;
      packet_t packet;
      int fnord;
      int sock;

      if (( sock = socket( AF_INET, SOCK_STREAM, 0 )) <= 0 )
      {
         strerror_r( errno, errBuf, 128 );
         printf("socket failed\n");
         return 0;
      }

      memset( &cwopServerAddr, 0, sizeof( cwopServerAddr ));

      if ( ( gethostbyname_r( "127.0.0.1",
                  &host, (char*)&gethostBuf, sizeof(gethostBuf), &res, &fnord )) != 0 )
      {
         printf("gethostbyname failed\n");
         return 0;
      }

      // Set the out IP address
      memcpy( &cwopServerAddr.sin_addr, host.h_addr, host.h_length );
      cwopServerAddr.sin_family = AF_INET;  
      cwopServerAddr.sin_port   = htons(9999);
      if ( connect( sock, ( const sockaddr *)&(cwopServerAddr), sizeof( struct sockaddr_in)) < 0 )
      {
         strerror_r( errno, errBuf, 128 );
         close( sock );
         printf("could not connect: %s\n", errBuf);
         return 0;
      }

      packet.reqType = getCurrent;
      packet.version = 0x01;
      packet.numArgs = 4;
      argType arg1;
      arg1.i = 0xdead;
      send(sock, (void*)&packet, sizeof(packet), MSG_EOR );
      send(sock, (void*)&arg1, sizeof( arg1), MSG_EOR );
      send(sock, (void*)&arg1, sizeof( arg1), MSG_EOR );
      send(sock, (void*)&arg1, sizeof( arg1), MSG_EOR );
      send(sock, (void*)&arg1, sizeof( arg1), MSG_EOR );
      close( sock );
}


