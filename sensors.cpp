# include "sensors.h"
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
 * SETTING UP SENSORS.
 */

/**
 * Initialize the sht31-D object.
 */
bool initSHT(Adafruit_SHT31 *sht) {
    if (!sht -> begin()) {
        debugln("Couldn't find SHT31.");
        return false;
    }

    debugln("SHT31-D found and initialized");
    return true;
}

/**
 * Initialize the bmp390 object.
 */
bool initBMP(TwoWire *wire, Adafruit_BMP3XX *bmp) {
    if (!bmp -> begin_I2C(0x77, wire)) {
        debugln("Couldn't find BMP390.");
        return false;
    }

    debugln('BMP390 found and initialized');

    /**
     * Set up oversampling and filter initialization
     */
    bmp -> setTemperatureOversampling(BMP3_OVERSAMPLING_32X);
    bmp -> setPressureOversampling(BMP3_OVERSAMPLING_32X);
    bmp -> setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_127);
    bmp -> setOutputDataRate(BMP3_ODR_50_HZ);
    return true;
}

/**
 * Initialize the SSD1306 display object.
 */
bool initDisplay(Adafruit_SSD1306 *display) {
    if (!display -> begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        debugln("Display allocation failed");
        return true; 
    }

    // resetDisplay(display);
    display -> setTextSize(1.8);
    debugln("Display Found!");
    display -> println("Welcome to: ");
    if (PROD) display -> println("Production");
    else display -> println("Development");
    display -> display();
    return true;
}

/**
 * De-initialize the camera.
 */
esp_err_t cameraTeardown() {
  esp_err_t err = ESP_OK;

  // Deinitialize camera
  err = esp_camera_deinit();

  // Display any errors associated with camera deinitialization
  if (err != ESP_OK) debugf("Error deinitializing camera: %s\n", esp_err_to_name(err));
  else debugf("Camera deinitialized successfully\n");
  
  debugln();
  return err;
}

/**
 * Set up the camera. 
 */
bool initCAM() {

    debugln("Setting up camera...");

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = CAMERA_CLK;
    config.frame_size = FRAMESIZE_QHD;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST; // Needs to be "CAMERA_GRAB_LATEST" for camera to capture.
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    /** if PSRAM keep res and jpeg quality.
    * Limit the frame size & quality when PSRAM is not available
    */
    if(!psramFound()) {
        debugln("Couldn't find PSRAM on the board!");
        config.frame_size = FRAMESIZE_SVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.jpeg_quality = 30;
    }

    /**
    * Initialize the camera
    */
    esp_err_t initErr = esp_camera_init(&config);
    if (initErr != ESP_OK) {
        debugf("Camera init failed with error 0x%x", initErr);
        debugln();
        return false;
    }

    sensor_t * s = esp_camera_sensor_get();
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 1);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 2);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 300);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)6);  // 0 to 6 (6 from "timelapse" example sjr, OK)
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 1);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

    delay(100);

    /**
    * Attempt to capture an image with the given settings.
    * If no capture, set camera status to false and de-init camera.
    */
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();
    if(!fb) {
        debugln("Camera capture failed");
        esp_err_t deinitErr = cameraTeardown();
        if (deinitErr != ESP_OK) debugf("Camera de-init failed with error 0x%x", deinitErr);
        debugln();
        esp_camera_fb_return(fb);
        return false;
    }

    esp_camera_fb_return(fb);
    debugln("Camera configuration complete!");
    return true;
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
 * Read and return the camera image.
 */
camera_fb_t* read() {
    camera_fb_t * fb = NULL;
    for(int i = 0; i < 3; i++) {
        fb = esp_camera_fb_get();
        esp_camera_fb_return(fb);
        delay(100);
    }
    fb = esp_camera_fb_get();
    return fb;
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
        Serial.println("Error: Failed to read the log file.");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContent);
    
    // If deserialization fails, start with an empty document
    if (error) {
        Serial.print("Error deserializing JSON: ");
        Serial.println(error.c_str());
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
        Serial.println("Error: Unable to open log file for writing.");
        return;
    }

    // Serialize the updated JSON document to the file
    if (serializeJson(doc, file) == 0) {
        Serial.println("Error: Failed to write to log file.");
    }

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
        Serial.print("Error deserializing JSON: ");
        Serial.println(error.c_str());
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
