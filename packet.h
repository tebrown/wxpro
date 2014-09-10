typedef enum { getCurrent = 0, getValue, getTrend, getAll } requestType;
typedef struct 
{
   requestType reqType   __attribute__ ((packed));
   UINT8       version;
   UINT8       numArgs;
   UINT16      crc;
} packet_t;

typedef struct 
{
   union {
      UINT64 i;
      F64    f;
   };
} argType;
