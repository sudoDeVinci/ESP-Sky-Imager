# SkyDeVision Sky Imaging Station

The ESP32S3-based low-cost sky imaging station.

![Prototype](/images/station_prototype_modern.jpg)

Wirelessly sent environmental readings and sky images enable cloud separation and weather prediction.

![Cloud detection](/images/cloud_detection_result01.png)

# Features

## WLAN Configurability

Multiple WLAN access points' details can be stored in the `networks.json` configurations file. This is a JSON file with the template:

```json
{
    "networks": [
        {
            "SSID": "Your network name",
            "PASS": "Your network password"
        },
        {
            "SSID": "Another network name",
            "PASS": "Another network password"
        }
    ]
}
```

During setup, the station scans the area for access points, and will attempt to connect to access points with details matching those listed, starting from the top. You can see this in `wifiSetup` in [comm.cpp](/comm.cpp).

## Value Caching To reduce cost and traffic

The system implements multi-level caching to optimize performance and reduce network usage:

- NTP time synchronization data is cached for 6 hours
- QNH (atmospheric pressure) values from external APIs are cached for 2 hours
- When offline, readings and images are stored locally in structured log files
- All cached data is automatically synchronized when connectivity is restored

## File system agnostic usage

The station is designed to work with different storage options:

- Primarily uses SD_MMC (SD card) for storage when available
- Falls back to internal LittleFS if SD card is not detected
- Maintains consistent file operations across both file systems
- Automatically creates and manages required directories and files

## Graceful Failure

The system includes robust error handling mechanisms:

- Monitors all sensors and connectivity components
- Continues operation with limited functionality when components fail
- Implements timeouts for external communications
- Logs errors and retries operations with exponential backoff
- Automatically recovers from most error conditions without manual intervention

## Offline/Online modes

The station seamlessly adapts to changing network conditions:

- In online mode: transmits data in real-time to configured server endpoints
- In offline mode: stores all readings and images locally on the SD card
- Automatically syncs cached data when connectivity is restored
- Maintains accurate timekeeping during extended offline periods
- Optimizes power usage based on network availability

## Power Management

- Implements deep sleep between readings to conserve battery
- Configurable sleep durations with different day/night schedules
- Wakes only during daylight hours for meaningful sky images
- Dynamically adjusts power states based on sensor requirements

## *COMING SOON*: Configurations Portal
- Web-based configuration interface
- Real-time sensor monitoring dashboard
- Remote firmware updates
- More advanced scheduling options

## Hardware Requirements

- Compatible ESP32-S3 with OV5640 camera
- SHT31 temperature and humidity sensor
- BMP390 barometric pressure sensor
- SD card module (recommended 16GB+)
- Optional SSD1306 display for status information

## Setup Instructions

1. Connect hardware components according to the pin definitions in [camera_pins.h](/camera_pins.h)
2. Create a `networks.json` file with your WiFi credentials
3. Flash the firmware to your ESP32-S3 device
4. Place the device in a weather-protected enclosure with the camera facing the sky
5. The system will automatically connect, calibrate, and begin operation

## License

This project is licensed under the Business Source License - see the [LICENSE](/LICENSE) file for details.