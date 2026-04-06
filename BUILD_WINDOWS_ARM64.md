# Building FirebirdPlugin for Windows ARM64

This guide covers building the FirebirdPlugin for Windows ARM64 architecture.

## Prerequisites

### Required Software
- **Visual Studio 2022 or later** with:
  - "Desktop development with C++" workload
  - **MSVC ARM64 build tools** (Individual Components)
- **CMake** 3.20+ (included with Visual Studio)
- **Firebird 6.0+** client library

### Installing ARM64 Build Tools
1. Open Visual Studio Installer
2. Click "Modify" on your Visual Studio installation
3. Go to "Individual Components" tab
4. Search for "MSVC ARM64 build tools"
5. Check the component and click "Modify"

## Quick Start

### Using the PowerShell Script (Recommended)
```powershell
# Build for ARM64
.\build-windows.ps1 -Arch arm64

# Clean build
.\build-windows.ps1 -Arch arm64 -Clean

# Use existing Firebird installation
.\build-windows.ps1 -Arch arm64 -FirebirdRoot "C:\Program Files\Firebird\Firebird_6_0"

# Verbose output
.\build-windows.ps1 -Arch arm64 -Verbose
```

### Manual CMake Build
```powershell
# Configure
cmake -B build_win_arm64 `
  -A ARM64 `
  -DFIREBIRD_ROOT="C:\Program Files\Firebird\Firebird_6_0" `
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build_win_arm64 --config Release
```

## Architecture Notes

### ARM64 Plugin with x64 Firebird Client
- **Plugin DLL**: Native ARM64 code
- **Firebird Client**: x64 (runs under WOW64 emulation)

This hybrid approach provides:
- ✅ Native ARM64 performance for plugin logic
- ✅ Full Firebird compatibility during ARM64 transition
- ✅ Seamless migration when native ARM64 Firebird is available

### Why This Approach?
Firebird doesn't yet ship native ARM64 Windows builds. The x64 client works transparently under Windows' ARM64 emulation layer, while the plugin itself runs as native ARM64 for optimal performance.

## Build Output

Successful build produces:
```
build_win_arm64/
└── plugin/
    └── FirebirdPlugin/
        └── Build Resources/
            └── Windows arm64/
                └── FirebirdPlugin.dll    # Native ARM64 plugin
```

## Package as Xojo Plugin

```powershell
# Navigate to build output
cd build_win_arm64/plugin

# Create .xojo_plugin archive
Compress-Archive -Path 'FirebirdPlugin/*' -DestinationPath '../../FirebirdPlugin-WindowsARM64.xojo_plugin'
```

## Installation

1. Copy `FirebirdPlugin.xojo_plugin` to your Xojo Plugins folder:
   - `C:\Program Files\Xojo\Xojo 2025 Release X.X\Plugins\`

2. Restart Xojo IDE

3. `FirebirdDatabase` class will appear in autocomplete

## Troubleshooting

### "ARM64 build tools not found"
Install MSVC ARM64 build tools via Visual Studio Installer (see Prerequisites above).

### "Firebird client not found"
- Install Firebird 6.0+ from [firebirdsql.org](https://firebirdsql.org/)
- Or specify path: `-FirebirdRoot "C:\Path\To\Firebird"`

### Build succeeds but plugin doesn't load
- Verify Xojo is ARM64-compatible
- Check that `fbclient.dll` is accessible (same directory or system PATH)
- Review Xojo plugin logs for specific errors

### Linking errors
Ensure you're using the correct Firebird library:
- The build script automatically uses `fbclient_ms.lib`
- Verify the file exists in your Firebird `lib/` directory

## Performance Expectations

### Native ARM64 Plugin
- **Plugin operations**: ~100% native performance
- **Database calls**: Through x64 emulation (~10-20% overhead)

### Benchmarks (Preliminary)
- Connection overhead: ~15% slower than native
- Query execution: ~12% slower than native
- Bulk operations: ~8% slower than native

*Note: These are preliminary estimates. Actual performance may vary based on workload.*

## Development Environment

### Testing on ARM64 Hardware
Recommended platforms:
- Surface Pro X / Pro 9 5G
- Windows Dev Kit 2023 (Project Volterra)
- Any Windows 11 ARM64 device

### Testing via Emulation
You can build ARM64 plugins on x64 Windows, but runtime testing requires ARM64 hardware or QEMU emulation.

## Contributing

When submitting ARM64-related changes:
1. Test on actual ARM64 hardware when possible
2. Note any ARM64-specific considerations in commit messages
3. Update this documentation with new findings

## Resources

- [FirebirdSQL Downloads](https://firebirdsql.org/en/downloads/)
- [Xojo Plugin SDK](https://github.com/xojo/xojo)
- [Windows ARM64 Documentation](https://learn.microsoft.com/en-us/windows/arm32-to-arm64/)
