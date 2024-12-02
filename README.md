# Desk Scheduler - Pico firmware

This repository is part of the Desk Scheduler project and contains the firmware for the **Raspberry Pi Pico W**, using it as a Wi-Fi-enabled, customizable desk alert and notification system. The project is designed for use with the default Picobricks setup and supports a variety of desk alerts, alarms, and user notifications via an HTTP API.

## Table of Contents
- [Dependencies](#dependencies)
- [Notes](#notes)
- [Usage](#usage)
- [API Endpoints](#api-endpoints)
- [Known Issues](#known-issues)
- [License](#license)

## Dependencies
This project depends on the following libraries:
- [Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [pico-ssd1306](https://github.com/Harbys/pico-ssd1306)
- [WS2812](https://github.com/ForsakenNGS/Pico_WS2812)

## Notes
- It is highly recommended to import this repository using the [official Raspberry Pi Pico extension for VSCode](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico) as it handles configuration and building automatically. Other methods are possible but currently not supported.
- Before compiling, configure Wi-Fi credentials by adding the following in `build/CMakeCache.txt`:
  ```
  WIFI_SSID:INTERNAL=<your-ssid>
  WIFI_PASSWORD:INTERNAL=<your-password>
  ```

## Usage

Once the firmware is flashed and the Pico W is powered on, the system will attempt to connect to the configured Wi-Fi network. Upon successful connection, it will start listening for HTTP API calls.

The onboard button can be used to interact with specific alerts and notifications.

### API Endpoints

    /api/error
    Causes the Pico to display an error indefinitely.

    /api/errend
    Ends the error state triggered by /api/error and returns to idle.

    /api/prealarm
    Displays a pre-alarm warning, which can be dismissed.

    /api/alarm?position=<number>&melody=<character>
    Starts an alarm, displaying the specified position and playing the selected melody.

    /api/login?username=<name>
    Displays a login message with the specified username (up to 10 characters). Dismissible.

    /api/logout
    Displays a logout message. Dismissible.

### Known Issues

- The RTC may show 00:00 temporarily when initialized. This will automatically correct itself after syncing with an NTP server.
- The onboard clock operates in GMT+1. This is intentional and currently not configurable.
- The API uses plain HTTP and is accessible to anyone on the local network. Avoid sending sensitive data. Future versions may implement HTTPS or other security measures.

## License

This project is licensed under the BSD-3-Clause License. See the [LICENSE](https://github.com/danssolutions/desk-scheduler-pico/blob/main/LICENSE) file for details.
