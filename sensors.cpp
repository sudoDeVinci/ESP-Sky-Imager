# include "sensors.h"

bool PROD = false;
double SEALEVELPRESSURE_HPA = UNDEFINED;
unsigned long lastPressed = millis();

/**
This holds the various sensor functions that are used.
-> SHT31-D Temperature and Humidity Sensor
-> BMP390 Temperature and Pressure Sensor
-> SSD 1306 DISPLAY
 */


/**
 * MATH RELATED FUNCTIONS
 */

/**
 * Calculate the median of a sorted array.
 */
double median(double* sortedArray, uint16_t size) {
  if (size % 2 == 0) return (sortedArray[size / 2 - 1] + sortedArray[size / 2]) / 2.0;
  else return sortedArray[size / 2];
}

/**
 * Calculate the mean of an arbitrary array.
 */
double mean(double* array, uint16_t size) {
  double sum = 0;
  for (int i = 0; i < size; i++) sum += array[i];
  return sum / size;
}

/**
 * Removes the outliers from an array and returns the mean of the remaining values.
 */
double removeOutliersandGetMean(double* dataArr, uint16_t size) {
    // Sort the array
    std::sort(dataArr, dataArr + size);

    // Calculate the IQR
    const uint16_t Q1_index = size / 4;
    const uint16_t Q3_index = (3 * size) / 4;
    const double Q1 = median(dataArr, Q1_index == 0 ? 1 : Q1_index);
    const double Q3 = median(dataArr + Q3_index, size - Q3_index);
    const double IQR = Q3 - Q1;

    // Calculate the lower and upper bounds
    const double lowerBound = Q1 - 1.5 * IQR;
    const double upperBound = Q3 + 1.5 * IQR;

    double sum = 0.0;
    int valid = 0;

    for (int i = 0;  i < size; i++) {
        if (dataArr[i] >= lowerBound && dataArr[i] <= upperBound) {
            sum += dataArr[i];
            valid ++;
        }
    }

    return sum / valid;
}

/**
 * Calculate dewpoint corrected for altitude. 
 */
double calcDP(double temperature, double humidity, double pressure, double altitude) {
    const double alpha = ((MAGNUS_A * temperature) / (MAGNUS_B + temperature)) + log(humidity / 100.0);
    const double dewPoint = (MAGNUS_B * alpha) / (MAGNUS_A - alpha); 
    return (dewPoint - (altitude / 1000.0));
}


/**
 * SENSOR READING FUNCTIONS
 */

/**
 * read the temperature and humidity from the SHT31-D sensor.
 */
void read(Adafruit_SHT31 *sht, double *out) {
    double h[SAMPLES];
    double t[SAMPLES];
    uint8_t errors = 0;
    uint8_t valid = 0;

    while (valid < SAMPLES && errors < 5) {
        h[valid] = sht -> readHumidity();
        t[valid] = sht -> readTemperature();
        if (isnan(h[valid]) || isnan(t[valid])) {
            errors++;
            continue;
        }
        else valid++;
        delay(50);
    }

    if( errors < 5 ) { 
        out[0] = removeOutliersandGetMean(h, valid);
        out[1] = removeOutliersandGetMean(t, valid);
    }
}

/**
 * Read the temperature and pressure from the BMP390 sensor.
 */
void read(Adafruit_BMP3XX *bmp, double *out) {
    
    if (! bmp -> performReading()) return;

    double presses[SAMPLES];
    double altids[SAMPLES];
    double tempers[SAMPLES];
    uint8_t errors = 0;
    uint8_t valid = 0;

    while ( valid < SAMPLES && errors < 5 ) {
        presses[valid] = bmp -> readPressure();
        altids[valid] = bmp -> readAltitude(SEALEVELPRESSURE_HPA);
        tempers[valid] = bmp -> readTemperature();
        if (isnan(presses[valid]) || isnan(altids[valid]) || isnan(tempers[valid])) {
            errors++;
            continue;
        }
        else valid++;
        delay(50);
    }

    if (errors < 5) {
        out[0] = removeOutliersandGetMean(altids, valid);
        out[1] = removeOutliersandGetMean(tempers, valid);
        out[2] = removeOutliersandGetMean(presses, valid);
    }
}

/**
 * Read all sensors and return a reading object untimestamped. 
 */
Reading readAll(Sensors::Status *stat, Adafruit_SHT31 *sht, Adafruit_BMP3XX *bmp) {
    Reading reading;
    double* ht = new double[2]{UNDEFINED, UNDEFINED};
    double* atp = new double[3]{UNDEFINED, UNDEFINED, UNDEFINED};
    double temperature, humidity, pressure, altitude, dewpoint;

    if (stat -> SHT) read(sht, ht);
    if (stat -> BMP) read(bmp, atp);
    
    /*
    * Assign temperature. Ideally this is from the SHT.
    * In case the BMP is down, get from BMP.
    */
    if (stat -> SHT && ht[1] != UNDEFINED) temperature = ht[1];
    else if (stat -> BMP && atp[1] != UNDEFINED) temperature = atp[1];
    else temperature = UNDEFINED;

    if (temperature != UNDEFINED &&
        humidity !=UNDEFINED &&
        pressure !=UNDEFINED && 
        altitude !=UNDEFINED) {
            double dewpoint = calcDP(temperature, humidity, pressure, altitude);
            reading.dewpoint = dewpoint;
    }

    if(temperature != UNDEFINED) reading.temperature = temperature;
    if(humidity != UNDEFINED) reading.humidity = humidity;
    if(pressure != UNDEFINED) reading.pressure = pressure;

    return reading;
}

/**
 * Append a reading object to the log file.
 */
void appendReading(fs::FS &fs, const Reading* reading) {
    // Read the existing content of the log file
    const char* fileContent = readFile(SD_MMC, LOG_FILE);
    
    // Check if the file content is valid
    if (fileContent == nullptr) {
        debugln("Error: Failed to read the log file.");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContent);
    
    // If deserialization fails, start with an empty document
    if (error) {
        debug("Error deserializing JSON: ");
        debugln(error.c_str());
        doc.createNestedArray("readings");
    }

    // Get the array of readings
    JsonArray readings = doc["readings"];

    // Create a new nested object for the new reading
    JsonObject newReading = readings.createNestedObject();
    newReading["timestamp"] = reading->timestamp;
    newReading["temperature"] = reading->temperature;
    newReading["humidity"] = reading->humidity;
    newReading["pressure"] = reading->pressure;
    newReading["dewpoint"] = reading->dewpoint;

    // Open the file for writing
    File file = fs.open(LOG_FILE, FILE_WRITE);
    
    // Check if the file opened successfully
    if (!file) {
        debugln("Error: Unable to open log file for writing.");
        return;
    }

    // Serialize the updated JSON document to the file
    if (serializeJson(doc, file) == 0) debugln("Error: Failed to write to log file.");

    // Close the file
    file.close();
}


/**
 * Read the log file and return an array of readings.
 * WARNING: DYNAMICALLY ALLOCATED HEAP ARRAY.
 */
ReadingLog readLog(fs::FS &fs) {
    const char* fileContent = readFile(SD_MMC, LOG_FILE);
    
    // Check if fileContent is valid
    if (fileContent == nullptr || strlen(fileContent) <= 1) {
        debugln("Error: Failed to read the log file or file is empty.");
        return {0, nullptr};
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContent);
    
    if (error) {
        debug("Error deserializing JSON: ");
        debugln(error.c_str());
        return {0, nullptr};
    }

    // Extract the array of readings from the parsed JSON
    JsonArray readings = doc["readings"];
    size_t numReadings = readings.size();

    // Dynamically allocate memory for Reading array on the heap
    Reading* log = new Reading[numReadings];

    // Iterate over the JSON array and populate the log array
    for (size_t i = 0; i < numReadings; i++) {
        JsonObject reading = readings[i];
        log[i] = Reading(
            reading["timestamp"] | "None",               // Use "None" as a default if not present
            reading["temperature"] | 0.0,                // Default to 0.0
            reading["humidity"] | 0.0,                   // Default to 0.0
            reading["pressure"] | 0.0,                   // Default to 0.0
            reading["dewpoint"] | 0.0                    // Default to 0.0
        );
    }

    return {numReadings, log};
}
