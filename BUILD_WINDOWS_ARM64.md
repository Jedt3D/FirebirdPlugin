# Building FirebirdPlugin for Windows ARM64

This guide covers building the FirebirdPlugin for Windows ARM64 architecture, including cross-compilation from ARM64 to other Windows architectures.

## Prerequisites

### Required Software
- **Visual Studio 2022 or later** with:
  - "Desktop development with C++" workload
  - **MSVC ARM64 build tools** (Individual Components)
- **CMake** 3.20+ (included with Visual Studio)
- **Firebird 6.0+** client library (for ARM64)
- **Firebird 5.0.3** client library (for Win32/x64 cross-compilation)

### Installing ARM64 Build Tools
1. Open Visual Studio Installer
2. Click "Modify" on your Visual Studio installation
3. Go to "Individual Components" tab
4. Search for "MSVC ARM64 build tools"
5. Check the component and click "Modify"

## Quick Start

### ARM64 Build (Native)
```powershell
# Configure (uses system Firebird 6.0)
cmake -B build_win_arm64 -A ARM64

# Build
cmake --build build_win_arm64 --config Release
```

### Cross-Compilation (ARM64 Host → x64/Win32)
```powershell
# Build for x64
cmake -B build_x64_fb5 `
  -A x64 `
  -DFIREBIRD_ROOT="C:/Users/worajedt/Firebird-5.0.3.1683-0-windows-x64-withDebugSymbols"
cmake --build build_x64_fb5 --config Release

# Build for Win32
cmake -B build_win32_fb5 `
  -A Win32 `
  -DFIREBIRD_ROOT="C:/Users/worajedt/Firebird-5.0.3.1683-0-windows-x86-withDebugSymbols"
cmake --build build_win32_fb5 --config Release
```

## Architecture-Specific Implementation

### ARM64 Plugin: Dynamic Loading
ARM64 builds use **runtime dynamic loading** to work with Firebird 6:

```cpp
// ARM64-specific implementation
#if defined(_WIN64) && defined(__aarch64__)
#include "FirebirdLoader.h"

// All Firebird API calls go through function pointers
#define isc_attach_database ptr_isc_attach_database
#define isc_dsql_execute ptr_isc_dsql_execute
// ... etc
#endif
```

**Why dynamic loading?**
- Firebird 5 doesn't have ARM64 builds
- Firebird 6 snapshot provides ARM64 support
- Allows runtime linking to `fbclient.dll`
- Avoids build-time architecture mismatches

### x64/Win32: Direct Linking
Non-ARM64 builds link directly to Firebird client libraries:
- Compile-time linking to `fbclient_ms.lib`
- No runtime loading overhead
- Standard Windows PE import tables

## Cross-Compilation Details

### Supported Cross-Compilations
```
ARM64 Host → ARM64 target (native) ✅
ARM64 Host → x64 target (cross)   ✅
ARM64 Host → Win32 target (cross) ✅
```

### Compiler Invocation
```bash
# ARM64 → ARM64 (native)
Hostarm64/arm64/cl.exe

# ARM64 → x64 (cross)
Hostarm64/x64/cl.exe

# ARM64 → x86 (cross to Win32)
Hostarm64/x86/cl.exe
```

### Firebird Client Requirements
| Target Architecture | Firebird Version | Build-Time Linking |
|-------------------|-----------------|-------------------|
| ARM64 | Firebird 6.0 | ❌ No (runtime loading) |
| x64 | Firebird 5.0.3 | ✅ Yes (fbclient_ms.lib) |
| Win32 | Firebird 5.0.3 | ✅ Yes (fbclient_ms.lib) |

## Build Outputs

### ARM64 Build
```
build_win_arm64/
├── Release/
│   └── FirebirdPlugin.dll    # 350 KB, PE32+ ARM64
└── FirebirdPlugin.xojo_plugin  # Xojo plugin package
```

### x64 Build
```
build_x64_fb5/
├── Release/
│   └── FirebirdPlugin.dll    # 349 KB, PE32+ x64
└── FirebirdPlugin.xojo_plugin  # Xojo plugin package
```

### Win32 Build
```
build_win32_fb5/
├── Release/
│   └── FirebirdPlugin.dll    # 286 KB, PE32 i386
└── FirebirdPlugin.xojo_plugin  # Xojo plugin package
```

## Installation

1. Copy the appropriate `.xojo_plugin` to your Xojo Plugins folder:
   - `C:\Program Files\Xojo\Xojo 2025 Release X.X\Plugins\`

2. Install matching Firebird client:
   - **ARM64 plugin**: Firebird 6.0 ARM64 client
   - **x64 plugin**: Firebird 5.0.3 x64 client
   - **Win32 plugin**: Firebird 5.0.3 x86 client

3. Restart Xojo IDE

4. `FirebirdDatabase` class will appear in autocomplete

## Troubleshooting

### "ARM64 build tools not found"
Install MSVC ARM64 build tools via Visual Studio Installer.

### "library machine type 'ARM64' conflicts with target machine type 'x64'"
You're using the wrong Firebird client library. Ensure:
- x64 builds use x64 Firebird libraries
- Win32 builds use x86 Firebird libraries
- ARM64 builds use dynamic loading (no library linking)

### "unresolved external symbol" errors
For x64/Win32 builds:
- Verify `-DFIREBIRD_ROOT` points to correct architecture
- Check that `fbclient_ms.lib` exists in the lib directory
- Ensure library architecture matches target (x86 for Win32, x64 for x64)

For ARM64 builds:
- Ensure `FirebirdLoader.cpp` is being compiled
- Check that `__aarch64__` is defined during compilation
- Verify runtime `fbclient.dll` is accessible

### Build succeeds but plugin doesn't load
- Verify Firebird client architecture matches plugin architecture
- Check that `fbclient.dll` is in PATH or same directory
- Review Xojo plugin logs for specific errors
- For ARM64: Ensure Firebird 6 ARM64 client is installed

## Performance Expectations

### Native ARM64 Plugin
- **Plugin operations**: 100% native performance
- **Database operations**: Native Firebird 6 ARM64 performance
- **Memory overhead**: Minimal (dynamic loading adds <1%)

### Cross-Compiled Plugins
- **x64 plugin**: Native x64 performance
- **Win32 plugin**: Native x86 performance (runs under WOW64 on x64 Windows)

### All Architectures
- **Connection overhead**: Similar across architectures
- **Query execution**: Depends on Firebird client version
- **Bulk operations**: Efficient data transfer

## Development Environment

### Recommended ARM64 Hardware
- Surface Pro X / Pro 9 5G
- Windows Dev Kit 2023 (Project Volterra)
- Any Windows 11 ARM64 device
- Snapdragon X Elite laptops (2024+)

### Cross-Compilation Benefits
- **Single build host**: ARM64 machine builds all Windows targets
- **Consistent toolchain**: Same Visual Studio, same CMake version
- **Faster builds**: Native ARM64 compilation is fast
- **Unified workflow**: One script/command for all architectures

## Contributing

When submitting changes:
1. **Test on target architecture** when possible
2. **Verify cross-compilation** builds for all targets
3. **Update architecture-specific code** carefully:
   - ARM64: `#if defined(_WIN64) && defined(__aarch64__)`
   - x64/Win32: Direct Firebird API calls
4. **Document architecture limitations** in commit messages

## Architecture-Specific Code Patterns

### Pattern 1: ARM64 Dynamic Loading
```cpp
// Only for ARM64
#if defined(_WIN64) && defined(__aarch64__)
#include "FirebirdLoader.h"
#define isc_function ptr_isc_function
#endif
```

### Pattern 2: Universal Code
```cpp
// Works on all architectures
void FBDatabase::connect() {
    isc_attach_database(...);  // Direct call or macro
}
```

### Pattern 3: Architecture Detection
```cpp
#if defined(__aarch64__)
    // ARM64-specific code
#elif defined(_M_X64) || defined(__x86_64__)
    // x64-specific code
#elif defined(_M_IX86) || defined(__i386__)
    // x86-specific code
#endif
```

## Resources

- [FirebirdSQL Downloads](https://firebirdsql.org/en/downloads/)
- [Firebird 6.0 Release Notes](https://firebirdsql.org/en/firebird-6-0-release-notes/)
- [Xojo Plugin SDK](https://github.com/xojo/xojo)
- [Windows ARM64 Documentation](https://learn.microsoft.com/en-us/windows/arm32-to-arm64/)
- [Cross-Compilation Guide](https://learn.microsoft.com/en-us/cpp/build/vcswx64-anatomy-of-a-cross-platform-completion?view=msvc-170)

## Build Status

✅ **All architectures production-ready** (2026-04-07)
- ARM64: Firebird 6.0 with dynamic loading
- x64: Firebird 5.0.3 with direct linking
- Win32: Firebird 5.0.3 with direct linking
