# Environmental Sensor Module Example

This is a complete, working example of a custom web module for the WebPlatform library. It demonstrates all key features of the web module system including authentication, configuration management, real-time updates, and RESTful APIs.

## Features Demonstrated

- **Complete IWebModule Implementation**: Proper header/source file structure
- **Mixed Authentication Requirements**: Different endpoints with different security needs
- **Real-time Data Updates**: JavaScript-based auto-refresh functionality  
- **Configuration Management**: Web forms with CSRF protection
- **RESTful API Design**: JSON APIs for external system integration
- **Module State Management**: Persistent configuration and sensor state
- **Professional Structure**: Proper C++ class design with public/private separation

## File Structure

```
lib/custom_module/
├── environmental_sensor_module.h    # Header file with class declaration
└── environmental_sensor_module.cpp  # Implementation file

src/
└── main.cpp                         # Example main application (custom_module_main.cpp)
```

## Quick Start

1. **Copy the module files** to your project:
   ```bash
   mkdir -p lib/environmental_sensor/
   cp environmental_sensor_module.h lib/environmental_sensor/
   cp environmental_sensor_module.cpp lib/environmental_sensor/
   ```

2. **Use the example main.cpp**:
   ```cpp
   #include <Arduino.h>
   #include <web_platform.h>
   #include "environmental_sensor_module.h"
   
   void setup() {
     // ... setup code from custom_module_main.cpp
   }
   ```

3. **Build and upload** to your ESP32

4. **Access the interface**:
   - Main sensor dashboard: `http://yourdevice.local/sensors/`
   - Configuration page: `http://yourdevice.local/sensors/config`
   - API endpoints: `http://yourdevice.local/sensors/api/current`

## Authentication Levels Demonstrated

### Local Network Only
- **Main Dashboard** (`/`): Accessible to anyone on local network
- Shows current readings and basic controls

### Session Authentication Required
- **Configuration Page** (`/config`): Requires web login
- **Configuration API** (`/api/config`): Form submission with CSRF protection
- **Current Data API** (`/api/current`): Real-time data for logged-in users

### API Token Required
- **Data API** (`/api/data`): Complete sensor information for external systems
- **Control API** (`/api/control`): Sensor control commands

### Mixed Authentication
Most endpoints accept multiple auth methods for flexibility:
- Session cookies (web interface users)
- API tokens (external systems)
- Page tokens (CSRF protection)

## API Endpoints

### Public Access (Local Network)
```bash
GET /sensors/                    # Main dashboard (HTML)
```

### Session Required
```bash
GET /sensors/config              # Configuration page (HTML)
GET /sensors/api/current         # Current sensor readings (JSON)
POST /sensors/api/config         # Update configuration (JSON)
```

### API Token Required
```bash
GET /sensors/api/data            # Complete sensor data (JSON)
POST /sensors/api/control        # Control commands (JSON)
```

### Example API Usage

**Get current readings:**
```bash
curl "http://yourdevice.local/sensors/api/current"
```

**Control sensor (with API token):**
```bash
curl -H "Authorization: Bearer YOUR_TOKEN" \
     -X POST \
     -d "command=refresh" \
     "http://yourdevice.local/sensors/api/control"
```

## API Documentation

The module uses the new OpenAPIDocumentation system to document endpoints:

```cpp
ApiRoute(
    "/current", WebModule::WM_GET,
    [this](WebRequest &req, WebResponse &res) {
      getCurrentDataHandler(req, res);
    },
    {AuthType::SESSION, AuthType::PAGE_TOKEN},
    EnvironmentalSensorDocs::createGetCurrentReadings()
),
```

This approach:
- Separates documentation from implementation code
- Provides structured API documentation that can be consumed by OpenAPI tools
- Creates consistency across all module APIs

## Configuration Options

The module includes a web-based configuration interface with the following settings:

- **Sensor Location**: Descriptive location name
- **Enable/Disable Sensor**: Toggle sensor readings
- **Temperature Threshold**: Alert threshold for temperature (°C)
- **Humidity Threshold**: Alert threshold for humidity (%)
- **Alert Notifications**: Enable/disable threshold alerts

## Real-World Adaptation

This example uses simulated sensor readings. To adapt for real sensors:

1. **Replace simulation** in `updateSensorReadings()`:
   ```cpp
   void EnvironmentalSensorModule::updateSensorReadings() {
     if (!sensorEnabled) return;
     
     // Replace with actual sensor reading code
     // temperature = realSensor.readTemperature();
     // humidity = realSensor.readHumidity();
     
     lastReading = millis();
   }
   ```

2. **Add sensor initialization** in `begin()`:
   ```cpp
   void EnvironmentalSensorModule::begin() {
     Serial.println("Environmental Sensor Module initialized");
     // realSensor.begin();
     updateSensorReadings();
   }
   ```

3. **Add required libraries** to your `platformio.ini`:
   ```ini
   lib_deps = 
     adafruit/DHT sensor library@^1.4.4
     adafruit/Adafruit Unified Sensor@^1.1.9
   ```

## Class Design Features

### Public Interface
- **Lifecycle Methods**: `begin()`, `handle()` for module management
- **Data Access**: Getters for current sensor values
- **Configuration**: Setters for programmatic configuration
- **Web Routes**: Standard IWebModule interface implementation

### Private Implementation  
- **Sensor Data**: Private member variables for readings
- **Route Handlers**: Separate methods for each HTTP endpoint
- **HTML Generation**: Methods that create web interface responses
- **JSON APIs**: Methods that return structured data

### Thread Safety
- Uses Arduino's single-threaded model
- All updates happen in `handle()` method
- No shared state concerns

## Development Notes

This module serves as a template for creating your own web-enabled device modules. Key patterns to follow:

1. **Separate header/implementation** files for maintainability
2. **Clear public API** with appropriate getters/setters
3. **Route handlers** as private methods for organization  
4. **Mixed authentication** to support both web and API users
5. **Proper error handling** with meaningful responses
6. **Configuration persistence** (extend with StorageManager if needed)

## License

This example is provided as educational material and may be used freely in your projects.