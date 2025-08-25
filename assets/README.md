# WebPlatform Assets

This directory contains web assets (HTML, CSS, JS) for the WebPlatform module.

## Files

- `config_portal_html.h` - HTML templates for WiFi configuration portal
- `config_portal_css.h` - CSS styles for the configuration portal
- `config_portal_js.h` - JavaScript enhancements for the configuration portal

## Usage

These assets are compiled into the firmware and served by the WebPlatform module. 
They provide the user interface for configuring WiFi and managing the device.

## Security Notes

The configuration portal can operate in two modes:
1. HTTP mode (when certificates are not available)
2. HTTPS mode (when certificates are available)

In HTTPS mode, WiFi credentials are transmitted securely. In HTTP mode, credentials 
are transmitted in the clear, but only directly between the device and the connecting 
client, which mitigates most security concerns in a typical setup scenario.