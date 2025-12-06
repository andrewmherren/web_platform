Import("env")
import platform

# Add platform-specific coverage link flags
# Note: --coverage is already in build_flags for compilation
system = platform.system()

print(f"🔧 Coverage flags script running on: {system}")

if system == "Darwin":  # macOS
    # On macOS with clang, --coverage handles both compile and link
    env.Append(LINKFLAGS=["--coverage"])
    print("✅ Applied macOS coverage link flags")
elif system == "Linux":
    # On Linux with GCC, add explicit gcov linking
    # --coverage in build_flags handles -fprofile-arcs and -ftest-coverage
    env.Append(LINKFLAGS=["--coverage", "-lgcov"])
    print("✅ Applied Linux coverage link flags")
elif system == "Windows":
    # On Windows with GCC, add explicit gcov linking
    env.Append(LINKFLAGS=["--coverage", "-lgcov"])
    print("✅ Applied Windows coverage link flags")
else:
    print(f"⚠️  Unknown platform: {system}, skipping coverage link flags")
