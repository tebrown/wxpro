/* including netdb.h suppresses warnings from the use of hostent */
#include <netdb.h>

#ifndef __LINUX__
#define LOCAL_GETHOSTBYNAME_R

int gethostbyname_r (const char *name,
     struct hostent *ret,
     char *buf,
     size_t buflen,
     struct hostent **result,
     int *h_errnop);

#endif /* gethostbyname_r */

