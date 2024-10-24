#ifndef WAPPER_H
#define WRAPPER_H
#include "comm.h"

/**
 * Try to get the current time.
 * 1. Check the cache for the last time we queried NTP server. - if it is over 6 hours, query the server.
 * 1.2. Query the NTP server - Update cache if succesfful.
 * 2. Get the current time for the system.
 * 
 * @param  fs: The file system reference to use for the cache.
 * @param  now: The time struct to fill with the current time.
 */
void fetchCurrentTime(fs::FS &fs, tm *now, Sensors::Status *status);


#endif // COMM_H
