#ifndef WAPPER_H
#define WRAPPER_H
#include "comm.h"

/**
 * Try to get the current time from the NTP server.
 * 1. Read the current system time.
 * 2. Check the cache for the last time we queried NTP server. - if it is over 6 hours, query the server.
 * 3. Query the NTP server - Update cache if succesfful.
 * 
 * @param  fs: The file system reference to use for the cache.
 * @param now: The time struct to fill with the current time.
 * @param stat: The status struct to check if we have wifi connection.
 */
void fetchCurrentTime(fs::FS &fs, tm *now, Sensors::Status *stat);

/**
 * Get the QNH from the api if there is internet.
 * 1. read the cache for the last time we queried the api.
 * 2. If the cache is older than 2 hours, query the api.
 * 3. If connection, get the QNH from the api, then update the cache.
 * 4. If no connection, return the cached value.
 * 
 * @param fs: The file system reference to use for the cache.
 * @param timestamp: The timestamp to update the cache with.
 * @param network: The network struct to check if we have wifi connection.
 */
double fetchQNH(fs::FS &fs, tm* now, NetworkInfo *network);


#endif // COMM_H
