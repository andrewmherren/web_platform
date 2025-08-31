#ifndef LOGIN_PAGE_HTML_H
#define LOGIN_PAGE_HTML_H

#include <Arduino.h>

const char LOGIN_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Login - {{DEVICE_NAME}}</title>
  <link rel="stylesheet" href="/assets/styles.css">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="csrf-token" content="{{csrfToken}}">
</head>
<body>
  <div class="container">
    <h1>Login</h1>
    <form method="post" action="/login?redirect={{redirectUrl}}">
      <input type="hidden" name="_csrf" value="{{csrfToken}}">
      <div class="form-group">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" class="form-control" required>
      </div>
      <div class="form-group">
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" class="form-control" required>
      </div>
      <button type="submit" class="btn btn-primary">Login</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

const char LOGIN_PAGE_ERROR_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Login - {{DEVICE_NAME}}</title>
  <link rel="stylesheet" href="/assets/styles.css">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="csrf-token" content="{{csrfToken}}">
</head>
<body>
  <div class="container">
    <h1>Login</h1>
    <div class="card error">Invalid username or password</div>
    <form method="post" action="/login?redirect={{redirectUrl}}">
      <input type="hidden" name="_csrf" value="{{csrfToken}}">
      <div class="form-group">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" class="form-control" value="{{username}}" required>
      </div>
      <div class="form-group">
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" class="form-control" required>
      </div>
      <button type="submit" class="btn btn-primary">Login</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

#endif // LOGIN_PAGE_HTML_H