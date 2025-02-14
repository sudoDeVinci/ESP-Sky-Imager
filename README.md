# SkyDeVision Sky Imaging Station

The ESP32S3-based low-cost sky imaging station. Wirelessly sends environmental readings and sky image to server address.

## Features

### WLAN Configurability

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

### Value Caching To reduce cost and traffic

We cache values in our file system temporarily in multiple places,  

### File system agnostic usage.
### Graceful Failure
### Offline/Online modes

### *COMING SOON*: Configurations Portal