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
 * 
 * @return double: The QNH value in hPa.
 */
double fetchQNH(fs::FS &fs, tm* now, NetworkInfo *network);

/**
 * Send the readings to the server.
 * 1. Get the QNH.
 * 2. Get the sensor readings.
 * 3. check if the site is reachable.
 * 3.1. If not, save the readings to the log file.
 * 3.2. If yes, send the statuses, readings & image to the server.
 * 3.2.1. Send the logfile of readings to the server.
 * 
 * @param fs: The file system reference to use for the cache.
 * @param now: The time struct containing the current time.
 * @param sensors: The sensors struct containing the sensor objects &statuses.
 * @param network: The network struct to use the wifi connection.
 */
void serverInterop(fs::FS &fs, tm* now, Sensors* sensors, NetworkInfo* network);

/**
 * Send statuses of sensors to HOST on specified PORT.
 * 
 * @param https: The HTTPClient object to use for the request.
 * @param network: The network struct to use the wifi connection.
 * @param stat: The status struct to send to the server.
 * @param reading: The reading struct to send to the server.
 * @param img: The camera_fb_t struct w/ the image to send to the server.
 */
void sendData(HTTPClient* http, 
              NetworkInfo* network, 
              Reading* reading, 
              Sensors::Status* stat, 
              camera_fb_t* img);




#endif // COMM_H
