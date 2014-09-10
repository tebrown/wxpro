// http://www.cygwin.com/ml/cygwin/2004-04/msg00532.html
#include <netdb.h>
#include <sys/socket.h>
#include "gethostbyname_r.h"
#include <string.h>
#include <pthread.h>

#ifdef LOCAL_GETHOSTBYNAME_R
/* duh? ERANGE value copied from web... */
#define ERANGE 34
int gethostbyname_r (const char *name,
     struct hostent *ret,
     char *buf,
     size_t buflen,
     struct hostent **result,
     int *h_errnop) {

 int hsave;
 struct hostent *ph;
 static pthread_mutex_t __mutex = PTHREAD_MUTEX_INITIALIZER;
 pthread_mutex_lock(&__mutex); /* begin critical area */
    hsave = h_errno;
    ph = gethostbyname(name);
    *h_errnop = h_errno; /* copy h_errno to *h_herrnop */
    if (ph == NULL) {
  *result = NULL;
 } else {
  char **p, **q;
  char *pbuf;
  size_t nbytes=0;
  int naddr=0, naliases=0;
  /* determine if we have enough space in buf */

  /* count how many addresses */
  for (p = ph->h_addr_list; *p != 0; p++) {
   nbytes += ph->h_length; /* addresses */
   nbytes += sizeof(*p); /* pointers */
   naddr++;
  }
  nbytes += sizeof(*p); /* one more for the terminating NULL */

  /* count how many aliases, and total length of strings */

  for (p = ph->h_aliases; *p != 0; p++) {
   nbytes += (strlen(*p)+1); /* aliases */
   nbytes += sizeof(*p);  /* pointers */
   naliases++;
  }
  nbytes += sizeof(*p); /* one more for the terminating NULL */

  /* here nbytes is the number of bytes required in buffer */
  /* as a terminator must be there, the minimum value is ph->h_length */
  if(nbytes > buflen) {
   *result = NULL;
   pthread_mutex_unlock(&__mutex); /* end critical area */
   return ERANGE; /* not enough space in buf!! */
  }

  /* There is enough space. Now we need to do a deep copy! */
  /* Allocation in buffer:
   from [0] to [(naddr-1) * sizeof(*p)]:
       pointers to addresses
   at [naddr * sizeof(*p)]:
       NULL
   from [(naddr+1) * sizeof(*p)] to [(naddr+naliases) * sizeof(*p)] :
       pointers to aliases
   at [(naddr+naliases+1) * sizeof(*p)]:
       NULL
   then naddr addresses (fixed length), and naliases aliases (asciiz).
  */

  *ret = *ph;   /* copy whole structure (not its address!) */

  /* copy addresses */
  q = (char **)buf; /* pointer to pointers area (type: char **) */
  ret->h_addr_list = q; /* update pointer to address list */
  pbuf = buf + ((naddr+naliases+2)*sizeof(*p)); /* skip that area */
  for (p = ph->h_addr_list; *p != 0; p++) {
   memcpy(pbuf, *p, ph->h_length); /* copy address bytes */
   *q++ = pbuf; /* the pointer is the one inside buf... */
   pbuf += ph->h_length; /* advance pbuf */
  }
  *q++ = NULL; /* address list terminator */

  /* copy aliases */

  ret->h_aliases = q; /* update pointer to aliases list */
  for (p = ph->h_aliases; *p != 0; p++) {
   strcpy(pbuf, *p); /* copy alias strings */
   *q++ = pbuf; /* the pointer is the one inside buf... */
   pbuf += strlen(*p); /* advance pbuf */
   *pbuf++ = 0; /* string terminator */
  }
  *q++ = NULL; /* terminator */

  strcpy(pbuf, ph->h_name); /* copy alias strings */
  ret->h_name = pbuf;
  pbuf += strlen(ph->h_name); /* advance pbuf */
  *pbuf++ = 0; /* string terminator */

  *result = ret;  /* and let *result point to structure */

 }
 h_errno = hsave;  /* restore h_errno */

 pthread_mutex_unlock(&__mutex); /* end critical area */

 return (*result == NULL);

}

#endif /* LOCAL_GETHOSTBYNAME_R */

