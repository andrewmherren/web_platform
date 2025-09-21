#include "interface/utils/route_variant.h"
#include "interface/web_module_interface.h"
#include <stdexcept>

RouteVariant::RouteVariant(const WebRoute &route) : type(WEB_ROUTE) {
  webRoute = new WebRoute(route);
}

RouteVariant::RouteVariant(const ApiRoute &route) : type(API_ROUTE) {
  apiRoute = new ApiRoute(route);
}

RouteVariant::RouteVariant(const RouteVariant &other) : type(other.type) {
  if (type == WEB_ROUTE) {
    webRoute = new WebRoute(*other.webRoute);
  } else {
    apiRoute = new ApiRoute(*other.apiRoute);
  }
}

RouteVariant &RouteVariant::operator=(const RouteVariant &other) {
  if (this != &other) {
    // Clean up current data
    if (type == WEB_ROUTE) {
      delete webRoute;
    } else {
      delete apiRoute;
    }

    // Copy new data
    type = other.type;
    if (type == WEB_ROUTE) {
      webRoute = new WebRoute(*other.webRoute);
    } else {
      apiRoute = new ApiRoute(*other.apiRoute);
    }
  }
  return *this;
}

RouteVariant::~RouteVariant() {
  if (type == WEB_ROUTE) {
    delete webRoute;
  } else {
    delete apiRoute;
  }
}

const WebRoute &RouteVariant::getWebRoute() const {
  if (type != WEB_ROUTE) {
    // Use Serial.println instead of throwing for embedded compatibility
    ERROR_PRINTLN("ERROR: RouteVariant is not a WebRoute");
    // Return a static dummy to avoid crash
    static WebRoute dummy("", WebModule::WM_GET, nullptr);
    return dummy;
  }
  return *webRoute;
}

const ApiRoute &RouteVariant::getApiRoute() const {
  if (type != API_ROUTE) {
    ERROR_PRINTLN("ERROR: RouteVariant is not an ApiRoute");
    static ApiRoute dummy("", WebModule::WM_GET, nullptr);
    return dummy;
  }
  return *apiRoute;
}

// Template specializations for helper functions
template <> bool holds_alternative<WebRoute>(const RouteVariant &v) {
  return v.isWebRoute();
}

template <> bool holds_alternative<ApiRoute>(const RouteVariant &v) {
  return v.isApiRoute();
}

template <> const WebRoute &get<WebRoute>(const RouteVariant &v) {
  return v.getWebRoute();
}

template <> const ApiRoute &get<ApiRoute>(const RouteVariant &v) {
  return v.getApiRoute();
}