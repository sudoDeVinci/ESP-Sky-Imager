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
 * @param now: The time struct to fill with the current time.
 * @param stat: The status struct to check if we have wifi connection.
 */
void fetchCurrentTime(fs::FS &fs, tm *now, Sensors::Status *stat);

/**
 * Get the QNH from the api if there is internet.
 * 1. Check if we have internet.
 * 2. If connection, get the QNH from the api., then update the cache.
 * 3. If no connection, return the cached value.
 * 
 * @param fs: The file system reference to use for the cache.
 * @param timestamp: The timestamp to update the cache with.
 * @param network: The network struct to check if we have wifi connection.
 */
double fetchQNH(fs::FS &fs, const char* timestamp, NetworkInfo *network);


#endif // COMM_H
