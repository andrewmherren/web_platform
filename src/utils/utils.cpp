#include "utils/utils.h"

// Helper function to normalize API paths (same logic as ApiRoute)
String Utils::normalizeApiPath(const String &path) {
  // If already starts with /api/, use as-is
  if (path.startsWith("/api/")) {
    return path;
  }
  // If starts with /api but no trailing slash, add the slash
  if (path.startsWith("/api") &&
      (path.length() == 4 || path.charAt(4) != '/')) {
    return path + "/";
  }
  // If starts with /, prepend /api
  if (path.startsWith("/")) {
    return "/api" + path;
  }
  // Otherwise prepend /api/
  return "/api/" + path;
}