#ifndef __UTILS_H__
#define __UTILS_H__

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include <string>


size_t readTimeout( int fd, void *buf, size_t count, time_t timeout );

int strnfcat( char *str, size_t size, const char *format, ... );

double getJulianDay( int day, int month, int year, int hour, int minute, int second );

double dms2deg( int hour, int min, float sec );

std::string deg2dms( double deg );

double rangeDegrees( double deg );

#endif // __UTILS_H__
