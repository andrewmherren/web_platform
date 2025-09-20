#ifndef WEB_MODULE_INTERFACE_WEBSERVER_TYPEDEFS_H
#define WEB_MODULE_INTERFACE_WEBSERVER_TYPEDEFS_H

#include <WebServer.h>
// Define WebServerClass as a concrete class derived from WebServer
class WebServerClass : public WebServer {
public:
  WebServerClass(uint16_t port = 80) : WebServer(port) {}
};

#endif // WEB_MODULE_INTERFACE_WEBSERVER_TYPEDEFS_H