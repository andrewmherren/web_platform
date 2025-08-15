#include "wifi_ap.h"
#include "wifi_ap_web_clean.h"
#include <ArduinoJson.h>

#if defined(ESP32)
#include <esp_timer.h>
#elif defined(ESP8266)
#include <Ticker.h>
#endif

#include "../../src/ssl_cert.h"
// Include SSL certificate helper

// Create global instance of WiFiManager
WiFiManager wifiManager; // WiFiManager implementation

WiFiManager::WiFiManager()
    : server(80), connectionState(WIFI_CONNECTING), callbackCalled(false),
      webInterfaceEnabled(true) {
  baseName = "Device";
  strcpy(apSSIDBuffer, "DeviceSetup");
  Serial.println("WiFiManager constructor called");
  Serial.print("Default AP SSID: ");
  Serial.println(apSSIDBuffer);
}

void WiFiManager::begin(const char *name, bool enableWebInterface) {
  // Set the base name and web interface setting
  baseName = name;

  // Create the AP SSID using the base name
  snprintf(apSSIDBuffer, sizeof(apSSIDBuffer), "%sSetup", name);

  Serial.print("Base name set to: ");
  Serial.println(baseName);
  Serial.print("AP SSID set to: ");
  Serial.println(apSSIDBuffer);

  webInterfaceEnabled = enableWebInterface;

  // Initialize serial communication
  Serial.begin(115200);
  Serial.println();
  Serial.println("WiFiManager starting...");

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Try to connect to WiFi if credentials exist
  if (connectToStoredWiFi()) {
    connectionState = WIFI_CONNECTED;
    Serial.println("Connected to WiFi network");

    // Call setup complete callback if it's set and hasn't been called yet
    if (setupCompleteCallback && !callbackCalled) {
      Serial.println("Calling WiFi setup complete callback");
      setupCompleteCallback();
      callbackCalled = true;
    }
  } else {
    // If WiFi connection failed, start the configuration portal
    Serial.println("Failed to connect to stored WiFi, starting config portal");
    setupConfigPortal();
    connectionState = WIFI_CONFIG_PORTAL;
  }
}

void WiFiManager::onSetupComplete(WiFiSetupCompleteCallback callback) {
  setupCompleteCallback = callback;

  // If we're already connected and haven't called the callback yet, call it
  // now
  if (connectionState == WIFI_CONNECTED && !callbackCalled) {
    setupCompleteCallback();
    callbackCalled = true;
  }
}

void WiFiManager::handle() {
  // Process web server requests
  server.handleClient();

  // If in config portal mode, handle DNS requests for captive portal
  if (connectionState == WIFI_CONFIG_PORTAL) {
    dnsServer.processNextRequest();
  }

  // Handle mDNS if connected
  if (connectionState == WIFI_CONNECTED) {
#ifdef ESP8266
    // Only ESP8266 needs explicit mDNS update calls
    MDNS.update();
#endif

    // Check if WiFi is still connected but mDNS might need restarting
    static unsigned long lastMdnsCheck = 0;
    unsigned long currentMillis = millis();

    // Check mDNS status every 30 seconds
    if (currentMillis - lastMdnsCheck > 30000) {
      lastMdnsCheck = currentMillis;

      // Try to restart mDNS if it's not responding
      if (!MDNS.begin(baseName)) {
        Serial.println("Restarting mDNS responder...");
        MDNS.end();
        if (MDNS.begin(baseName)) {
          Serial.println("mDNS responder restarted");
          MDNS.addService("http", "tcp", 80);
        }
      }
    }
  }

  // Check WiFi connection status and reconnect if needed
  if (connectionState == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost, trying to reconnect...");
    connectionState = WIFI_CONNECTING;
    callbackCalled = false; // Reset callback flag

    // Try to reconnect
    if (connectToStoredWiFi()) {
      connectionState = WIFI_CONNECTED;

      // Call setup complete callback again after reconnection
      if (setupCompleteCallback && !callbackCalled) {
        setupCompleteCallback();
        callbackCalled = true;
      }
    } else {
      setupConfigPortal();
      connectionState = WIFI_CONFIG_PORTAL;
    }
  }
}

WiFiConnectionState WiFiManager::getConnectionState() const {
  return connectionState;
}

WebServerClass &WiFiManager::getWebServer() { return server; }

void WiFiManager::registerRoutes(WebRouter &router, const char *basePath) {
  Serial.println("Registering WiFi management routes with Web Router...");

  String basePathStr = String(basePath);

  // API endpoints (always available) - simplified paths
  String statusPath = basePathStr + "/api/status";
  String scanPath = basePathStr + "/api/scan";
  String connectPath = basePathStr + "/api/connect";
  String disconnectPath = basePathStr + "/api/disconnect";
  String resetPath = basePathStr + "/api/reset";

  router.addRoute(statusPath.c_str(), HTTP_GET, [this](WebServerClass &server) {
    String response = this->handleWiFiStatusAPI();
    server.send(200, "application/json", response);
  });

  router.addRoute(scanPath.c_str(), HTTP_GET, [this](WebServerClass &server) {
    String response = this->handleWiFiScanAPI();
    server.send(200, "application/json", response);
  });

  router.addRoute(
      connectPath.c_str(), HTTP_POST, [this](WebServerClass &server) {
        String postBody = server.arg("plain");
        String response = this->handleWiFiConnectAPI(postBody);
        if (response.startsWith("ERROR:")) {
          server.send(400, "application/json", response.substring(6));
        } else {
          server.send(200, "application/json", response);
        }
      });

  router.addRoute(
      disconnectPath.c_str(), HTTP_POST, [this](WebServerClass &server) {
        String response = this->handleWiFiDisconnectAPI();
        if (response.startsWith("ERROR:")) {
          server.send(400, "application/json", response.substring(6));
        } else {
          server.send(200, "application/json", response);
        }
      });

  router.addRoute(resetPath.c_str(), HTTP_POST, [this](WebServerClass &server) {
    String response = this->handleWiFiResetAPI();
    server.send(200, "application/json", response);
  });

  // Web interface endpoints (optional)
  if (webInterfaceEnabled) {
    // Register at the base path directly (so /wifi becomes the main WiFi page)
    String wifiPath = basePathStr; // Just use the base path
    if (wifiPath.length() == 0 || wifiPath == "/") {
      wifiPath = "/wifi"; // Default to /wifi if base path is empty or root
    }
    String savePath = basePathStr + "/save";

    router.addRoute(
        wifiPath.c_str(), HTTP_GET, [this, wifiPath](WebServerClass &server) {
          Serial.println("WiFi page route handler called for: " + wifiPath);
          String response = this->handleRootPage();
          Serial.println("WiFi page content length: " +
                         String(response.length()));

          // Add cache control headers
          server.sendHeader("Cache-Control",
                            "no-cache, no-store, must-revalidate");
          server.sendHeader("Pragma", "no-cache");
          server.sendHeader("Expires", "0");

          server.send(200, "text/html", response);
          Serial.println("WiFi page response length: " +
                         String(response.length()));
        });

    router.addRoute(savePath.c_str(), HTTP_POST,
                    [this](WebServerClass &server) {
                      String postBody = server.arg("plain");
                      String response = this->handleSavePage(postBody);
                      server.send(200, "text/html", response);
                    });
  }
}

void WiFiManager::startConfigPortal() {
  Serial.println("Starting configuration portal...");

  // Reset callback flag
  callbackCalled = false;

  // Disconnect from any existing WiFi
  WiFi.disconnect();
  delay(100);

  // Set up the config portal
  setupConfigPortal();
  connectionState = WIFI_CONFIG_PORTAL;

  Serial.println("Configuration portal is now active");
}

void WiFiManager::resetSettings() {
  Serial.println("Resetting WiFi settings...");
  // Clear the config flag
  EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 0);
  EEPROM.commit();
}

void WiFiManager::setupConfigPortal() {
  Serial.println("Setting up configuration portal...");

  // Debug the AP SSID
  Serial.print("Starting AP with SSID: ");
  Serial.println(apSSIDBuffer);

  // Setup AP mode
  WiFi.mode(WIFI_AP);

  // Create AP network
  bool apStarted = WiFi.softAP(apSSIDBuffer, apPassword);

  if (apStarted) {
    Serial.println("AP started successfully");
  } else {
    Serial.println("Failed to start AP!");
  }

  IPAddress myIP;
  myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Setup captive portal DNS
  dnsServer.start(53, "*", myIP);

  // Note: In Phase 3, we no longer manage our own server
  // The Web Router will handle all HTTP traffic when WiFi connects
  // During config portal mode, we temporarily use the fallback server
  // for backward compatibility until full migration is complete

  // Stop the server to clear handlers
  server.stop();
  server.close();

  // Reset server handlers for config portal mode
  server.onNotFound([this]() { this->handleNotFound(); });

  // Set up API endpoints in config portal mode (legacy fallback)
  // Note: These routes match the frontend's expected paths
  server.on("/wifi/api/status", HTTP_GET, [this]() {
    Serial.println("Config portal: Handling /wifi/api/status");
    String response = this->handleWiFiStatusAPI();
    server.send(200, "application/json", response);
  });
  server.on("/wifi/api/scan", HTTP_GET, [this]() {
    Serial.println("Config portal: Handling /wifi/api/scan");
    String response = this->handleWiFiScanAPI();
    server.send(200, "application/json", response);
  });
  server.on("/wifi/api/connect", HTTP_POST, [this]() {
    Serial.println("Config portal: Handling /wifi/api/connect");
    String postBody = server.arg("plain");
    String response = this->handleWiFiConnectAPI(postBody);
    if (response.startsWith("ERROR:")) {
      server.send(400, "application/json", response.substring(6));
    } else {
      server.send(200, "application/json", response);
    }
  });
  server.on("/wifi/api/disconnect", HTTP_POST, [this]() {
    Serial.println("Config portal: Handling /wifi/api/disconnect");
    String response = this->handleWiFiDisconnectAPI();
    if (response.startsWith("ERROR:")) {
      server.send(400, "application/json", response.substring(6));
    } else {
      server.send(200, "application/json", response);
    }
  });
  server.on("/wifi/api/reset", HTTP_POST, [this]() {
    Serial.println("Config portal: Handling /wifi/api/reset");
    String response = this->handleWiFiResetAPI();
    server.send(200, "application/json", response);
  });

  if (webInterfaceEnabled) {
    // In AP mode, all routes should redirect to the WiFi setup page
    server.on("/", [this]() {
      String response = this->handleRootPage();
      server.send(200, "text/html", response);
    });
    server.on("/wifi", HTTP_GET, [this]() {
      String response = this->handleRootPage();
      server.send(200, "text/html", response);
    });
    server.on("/save", HTTP_POST, [this]() {
      String postBody = server.arg("plain");
      String response = this->handleSavePage(postBody);
      server.send(200, "text/html", response);
    });
    server.on("/wifi/save", HTTP_POST, [this]() {
      String postBody = server.arg("plain");
      String response = this->handleSavePage(postBody);
      server.send(200, "text/html", response);
    });
  }

  // Start the web server
  server.begin();
  Serial.println("HTTP server and captive portal started with API endpoints "
                 "(legacy mode)");
}

bool WiFiManager::connectToStoredWiFi() {
  String ssid, password;

  // Check if we have saved credentials
  if (!loadWiFiCredentials(ssid, password)) {
    Serial.println("No saved WiFi credentials found");
    return false;
  }

  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);

  // Attempt to connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection for 20 seconds
  int timeout = 20;
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    Serial.print(".");
    timeout--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());

    // Initialize mDNS with the base name
    Serial.print("Starting mDNS with hostname: ");
    Serial.println(baseName);

    // Stop any existing mDNS service
    MDNS.end();

    // Initialize mDNS with the base name
    if (MDNS.begin(baseName)) {
      Serial.print("mDNS responder started successfully: http://");
      Serial.print(baseName);
      Serial.println(".local");

      // Add service to mDNS
      MDNS.addService("http", "tcp", 80);

      // Additional debug
      Serial.println("HTTP service added to mDNS");
    } else {
      Serial.println("ERROR: Failed to start mDNS responder!");
      Serial.println("Device will be accessible via IP address only");
    }

    return true;
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi");
    return false;
  }
}

void WiFiManager::saveWiFiCredentials(const String &ssid,
                                      const String &password) {
  // Clear the EEPROM area for the new data
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }

  // Write SSID
  for (unsigned int i = 0; i < ssid.length(); i++) {
    EEPROM.write(WIFI_SSID_ADDR + i, ssid[i]);
  }

  // Write password
  for (unsigned int i = 0; i < password.length(); i++) {
    EEPROM.write(WIFI_PASS_ADDR + i, password[i]);
  }

  // Set the config flag (1 = configured)
  EEPROM.write(WIFI_CONFIG_FLAG_ADDR, 1);

  // Commit changes to EEPROM
  EEPROM.commit();

  Serial.println("WiFi credentials saved to EEPROM");
}

bool WiFiManager::loadWiFiCredentials(String &ssid, String &password) {
  // Check if WiFi is configured
  if (EEPROM.read(WIFI_CONFIG_FLAG_ADDR) != 1) {
    return false;
  }

  // Read SSID
  ssid = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(WIFI_SSID_ADDR + i);
    if (c == 0)
      break;
    ssid += c;
  }

  // Read password
  password = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(WIFI_PASS_ADDR + i);
    if (c == 0)
      break;
    password += c;
  }

  return true;
}

// API Handler Methods
String WiFiManager::handleWiFiStatusAPI() {
  Serial.println("WiFi status request received");

  String json = "{";
  json += "\"connected\":" +
          String(connectionState == WIFI_CONNECTED ? "true" : "false") + ",";
  json += "\"state\":\"" +
          String(connectionState == WIFI_CONNECTED       ? "connected"
                 : connectionState == WIFI_CONNECTING    ? "connecting"
                 : connectionState == WIFI_CONFIG_PORTAL ? "setup"
                                                         : "failed") +
          "\",";

  if (connectionState == WIFI_CONNECTED) {
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"hostname\":\"" + String(baseName) + ".local\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\"";
  } else {
    String ssid, password;
    if (loadWiFiCredentials(ssid, password)) {
      json += "\"saved_ssid\":\"" + ssid + "\"";
    } else {
      json += "\"saved_ssid\":null";
    }

    // Add AP info when in config portal mode
    if (connectionState == WIFI_CONFIG_PORTAL) {
      json += ",\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\"";
      json += ",\"ap_ssid\":\"" + String(apSSIDBuffer) + "\"";
    }
  }

  json += "}";

  Serial.println("Sending status: " + json);
  return json;
}

String WiFiManager::handleWiFiScanAPI() {
  Serial.println("Starting WiFi scan...");

  int n = 0;

  if (connectionState == WIFI_CONFIG_PORTAL) {
    // In AP mode, we need to temporarily switch to STA+AP mode to scan
    Serial.println("Scanning in AP mode - switching to STA+AP temporarily");
    WiFi.mode(WIFI_AP_STA);
    delay(100); // Small delay for mode switch
    n = WiFi.scanNetworks();
    // Switch back to AP only mode
    WiFi.mode(WIFI_AP);
    delay(100);
  } else {
    // In connected mode, we can scan directly
    n = WiFi.scanNetworks();
  }

  String json = "{\"networks\":[";
  for (int i = 0; i < n; ++i) {
    if (i > 0)
      json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";

#if defined(ESP32)
    // ESP32 uses different encryption type constants
    json += "\"encrypted\":" +
            String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
#elif defined(ESP8266)
    json += "\"encrypted\":" +
            String(WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false");
#endif
    json += "}";
  }
  json += "]}";

  Serial.print("WiFi scan completed, found ");
  Serial.print(n);
  Serial.println(" networks");

  return json;
}

String WiFiManager::handleWiFiConnectAPI(const String &postBody) {
  Serial.println("WiFi connect request received");
  Serial.println("POST body: " + postBody);

  // Handle both JSON and form data
  String ssid, password;

  if (postBody.length() > 0 && postBody.startsWith("{")) {
    // JSON format (from API call)
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, postBody);

    if (error) {
      Serial.println("JSON parsing failed: " + String(error.c_str()));
      return "ERROR:{\"success\":false,\"message\":\"Invalid JSON\"}";
    }

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
  } else {
    // Form data format (from traditional form submission)
    // Note: For direct parameter extraction, we'll need to parse manually
    int ssidStart = postBody.indexOf("ssid=");
    int passStart = postBody.indexOf("password=");

    if (ssidStart >= 0) {
      int ssidEnd = postBody.indexOf("&", ssidStart);
      if (ssidEnd < 0)
        ssidEnd = postBody.length();
      ssid = postBody.substring(ssidStart + 5, ssidEnd); // "ssid=" is 5 chars
      // URL decode basic characters
      ssid.replace("+", " ");
      ssid.replace("%20", " ");
    }

    if (passStart >= 0) {
      int passEnd = postBody.indexOf("&", passStart);
      if (passEnd < 0)
        passEnd = postBody.length();
      password =
          postBody.substring(passStart + 9, passEnd); // "password=" is 9 chars
      // URL decode basic characters
      password.replace("+", " ");
      password.replace("%20", " ");
    }
  }

  Serial.println("SSID: " + ssid);
  Serial.println("Password: [" + String(password.length()) + " characters]");

  if (ssid.length() == 0) {
    Serial.println("Error: SSID is empty");
    return "ERROR:{\"success\":false,\"message\":\"SSID is required\"}";
  }

  // Save credentials
  Serial.println("Saving WiFi credentials...");
  saveWiFiCredentials(ssid, password);

  Serial.println("Restarting device to connect to new WiFi...");

  // Schedule restart after response is sent
  // The web router will handle the delay
  delay(2000);
  ESP.restart();

  return "{\"success\":true,\"message\":\"Credentials saved. Device will "
         "restart.\"}";
}

String WiFiManager::handleWiFiDisconnectAPI() {
  if (connectionState == WIFI_CONNECTED) {
    WiFi.disconnect();
    connectionState = WIFI_CONNECTING;
    return "{\"success\":true,\"message\":\"Disconnected from WiFi\"}";
  } else {
    return "ERROR:{\"success\":false,\"message\":\"Not connected to WiFi\"}";
  }
}
String WiFiManager::handleWiFiResetAPI() {
  Serial.println("WiFi reset requested via API");
  Serial.println("Clearing WiFi credentials from EEPROM...");

  // Clear the stored WiFi credentials immediately
  resetSettings();

  Serial.println("WiFi settings cleared, scheduling restart...");

  // Return success response immediately
  String response = "{\"success\":true,\"message\":\"WiFi settings cleared. "
                    "Device will restart.\"}";

#if defined(ESP32)
  // Schedule restart after a brief delay to allow HTTP response to be sent
  // Use a simple task for ESP32
  xTaskCreate(
      [](void *parameter) {
        Serial.println("Reset task: Waiting 2 seconds before restart...");
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 second delay
        Serial.println("Reset task: Executing restart...");
        ESP.restart();
      },
      "wifi_reset_task", // Task name
      2048,              // Stack size
      nullptr,           // Parameters
      1,                 // Priority
      nullptr            // Task handle
  );
#elif defined(ESP8266)
  // For ESP8266, use a static Ticker for the restart delay
  static Ticker restartTicker;
  restartTicker.once_ms(2000, []() {
    Serial.println("Reset ticker: Executing restart...");
    ESP.restart();
  });
#endif

  return response;
}

// Web Interface Handler Methods (only used if web interface is enabled)
String WiFiManager::handleRootPage() {
  // In AP mode or connected mode, show WiFi management interface
  if (connectionState == WIFI_CONFIG_PORTAL) {
    Serial.println("Serving WiFi setup page in AP mode");
  } else {
    Serial.println("Serving WiFi management page in connected mode");
  }

  Serial.println("WiFi page content length: " +
                 String(strlen(WIFI_CONFIG_HTML)));
  return String(WIFI_CONFIG_HTML);
}

String WiFiManager::handleSavePage(const String &postBody) {
  // Parse form data from post body
  String ssid, password;

  int ssidStart = postBody.indexOf("ssid=");
  int passStart = postBody.indexOf("password=");

  if (ssidStart >= 0) {
    int ssidEnd = postBody.indexOf("&", ssidStart);
    if (ssidEnd < 0)
      ssidEnd = postBody.length();
    ssid = postBody.substring(ssidStart + 5, ssidEnd); // "ssid=" is 5 chars
    // URL decode basic characters
    ssid.replace("+", " ");
    ssid.replace("%20", " ");
  }

  if (passStart >= 0) {
    int passEnd = postBody.indexOf("&", passStart);
    if (passEnd < 0)
      passEnd = postBody.length();
    password =
        postBody.substring(passStart + 9, passEnd); // "password=" is 9 chars
    // URL decode basic characters
    password.replace("+", " ");
    password.replace("%20", " ");
  }

  // Validate input
  if (ssid.length() == 0) {
    return "ERROR: SSID is required";
  }

  // Save credentials
  saveWiFiCredentials(ssid, password);

  // Schedule restart after response is sent
  delay(2000);
  ESP.restart();

  return String(WIFI_SUCCESS_HTML);
}

void WiFiManager::handleNotFound() {
  if (connectionState == WIFI_CONFIG_PORTAL) {
    // In config portal mode, serve the WiFi setup page for all URLs
    Serial.print("Captive portal handling request: ");
    Serial.println(server.uri());

    // For captive portal detection on various devices
    if (server.hostHeader() != WiFi.softAPIP().toString()) {
      Serial.println("Captive portal - redirecting to setup page");
      server.sendHeader("Location",
                        "http://" + WiFi.softAPIP().toString() + "/wifi", true);
      server.send(302, "text/plain", "");
    } else {
      // Direct access to any URL on the device IP - serve the setup page
      Serial.println("Serving WiFi setup page");
      server.send_P(200, "text/html", WIFI_CONFIG_HTML);
    }
  } else {
    // In normal operation mode, return 404
    server.send(404, "text/plain", "Not found");
  }
}

// These are the legacy C-style functions that call through to our class
// instance They are kept for backward compatibility
void setupWiFiAP() { wifiManager.begin(); }

void loopWiFiAP() { wifiManager.handle(); }