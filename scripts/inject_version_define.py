"""
PlatformIO pre-build script that extracts version from library.json
and generates version_autogen.h with platform version macros.

This script ensures the web_platform version is automatically synchronized
with library.json, eliminating the need for manual version updates.
"""

Import("env")
import json
import os

# Get the project directory
project_dir = env.subst("$PROJECT_DIR")
library_json_path = os.path.join(project_dir, "library.json")

# Read library.json
try:
    with open(library_json_path, "r") as f:
        library_data = json.load(f)
except FileNotFoundError:
    env.Execute(f'echo "ERROR: library.json not found at {library_json_path}"')
    env.Exit(1)
except json.JSONDecodeError as e:
    env.Execute(f'echo "ERROR: Failed to parse library.json: {e}"')
    env.Exit(1)

# Extract version and platform name
platform_name = library_data.get("name", "unknown")
version = library_data.get("version")

if not version:
    env.Execute(f'echo "ERROR: No version field found in library.json"')
    env.Exit(1)

# Generate version_autogen.h
include_dir = os.path.join(project_dir, "include")
os.makedirs(include_dir, exist_ok=True)

version_header_path = os.path.join(include_dir, "version_autogen.h")

# Use module-specific header guard to avoid collisions when multiple modules are used
guard_name = f"{platform_name.upper().replace('-', '_')}_VERSION_AUTOGEN_H"

header_content = f"""// Auto-generated file - DO NOT EDIT
// Generated from library.json version field
// Platform: {platform_name}

#ifndef {guard_name}
#define {guard_name}

// Platform version from library.json
#define WEB_PLATFORM_VERSION_STR "{version}"
#define WEB_PLATFORM_VERSION {version.replace('.', '')}

#endif // {guard_name}
"""

# Write the header file
with open(version_header_path, "w") as f:
    f.write(header_content)

print(f"[{platform_name}] Generated version_autogen.h with version {version}")
