#ifndef __TEST_READER_H__
#define __TEST_READER_H__

#include "types.h"
#include "Debug.h"
#include "WeatherSource.h"

#define CR      0x0D
#define LF      0x0A
#define ACK     0x06
#define NAK     0x21
#define CANCEL  0x18

// Forward declarations

class TestReader:public WeatherSource
{
public:
    TestReader( Config &cfg );
    
    bool read( );
private:
    Debug dbg;

};

#endif // __TEST_READER_H__
