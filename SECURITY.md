# Security Guide

Practical checklist for taking a WebPlatform device from development to production.

## Before you deploy

- **Change the default admin credentials.** Every device boots with
  `admin` / `admin` (see [QUICK_START.md](QUICK_START.md)). Log in and set a
  real password immediately — this is the single most important step here.
- **Enable HTTPS.** WebPlatform auto-detects and serves HTTPS when
  certificates are present; see
  [HTTPS Configuration](GUIDE.md#https-configuration) in the full guide for
  generating and embedding certificates. Without it, credentials and session
  tokens travel in plaintext over the local network.
- **Review route auth requirements.** Every route declares its own
  `AuthRequirements` (`AuthType::NONE`, `SESSION`, `TOKEN`, `LOCAL_ONLY`,
  `PAGE_TOKEN`). Double-check any route you registered with `AuthType::NONE`
  is actually meant to be public.
- **Rotate and scope API tokens.** Tokens created under a user's account
  inherit that user's admin status. Prefer per-purpose tokens over reusing
  one broadly-scoped token, and revoke tokens you're no longer using.
- **Keep dependencies current.** WebPlatform, `web_platform_interface`, and
  ArduinoJson all receive fixes over time — pin to a specific version/commit
  deliberately (not a floating branch) and update on a schedule you control.

## Network exposure

- `AuthType::LOCAL_ONLY` routes still trust anything on the local network
  segment — don't rely on it as a substitute for authentication if the
  device sits on a shared or untrusted network.
- The WiFi captive portal used for first-time setup is unauthenticated by
  design (it has to be, to onboard a new device). Don't leave a device
  sitting in config-portal mode on a network you don't trust.

## Reporting a vulnerability

Open a [GitHub issue](https://github.com/andrewmherren/web_platform/issues)
or reach out via the repository owner's GitHub profile
([andrewmherren](https://github.com/andrewmherren)).
