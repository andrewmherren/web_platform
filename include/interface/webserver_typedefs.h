#ifndef WEB_MODULE_INTERFACE_WEBSERVER_TYPEDEFS_H
#define WEB_MODULE_INTERFACE_WEBSERVER_TYPEDEFS_H

#if defined(ESP32)
#include <WebServer.h>
// Define WebServerClass as a concrete class derived from WebServer
class WebServerClass : public WebServer {
public:
  WebServerClass(uint16_t port = 80) : WebServer(port) {}
};
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
// Define WebServerClass as a concrete class derived from ESP8266WebServer
class WebServerClass : public ESP8266WebServer {
public:
  WebServerClass(uint16_t port = 80) : ESP8266WebServer(port) {}
};
#endif

#endif // WEB_MODULE_INTERFACE_WEBSERVER_TYPEDEFS_H