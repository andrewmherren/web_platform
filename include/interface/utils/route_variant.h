#ifndef ROUTE_VARIANT_H
#define ROUTE_VARIANT_H

// Forward declarations to avoid circular includes
struct WebRoute;
struct ApiRoute;

// Simple variant-like class for C++11/14 compatibility
class RouteVariant {
private:
  enum Type { WEB_ROUTE, API_ROUTE } type;
  union {
    WebRoute *webRoute;
    ApiRoute *apiRoute;
  };

public:
  // Constructors
  RouteVariant(const WebRoute &route);
  RouteVariant(const ApiRoute &route);

  // Copy constructor
  RouteVariant(const RouteVariant &other);

  // Assignment operator
  RouteVariant &operator=(const RouteVariant &other);

  // Destructor
  ~RouteVariant();

  // Type checking
  bool isWebRoute() const { return type == WEB_ROUTE; }
  bool isApiRoute() const { return type == API_ROUTE; }

  // Getters
  const WebRoute &getWebRoute() const;
  const ApiRoute &getApiRoute() const;
};

// Helper functions to mimic std::variant API
template <typename T> bool holds_alternative(const RouteVariant &v);

template <typename T> const T &get(const RouteVariant &v);

#endif