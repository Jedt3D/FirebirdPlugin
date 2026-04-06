# Firebird 5.0.2 Setup Guide for ARM64 Build

## Quick Setup Instructions

### Option 1: Automated Download (Recommended)
The build script can automatically download Firebird 5.0.2:

```powershell
# Remove existing build directory
Remove-Item -Recurse -Force build_win_arm64 -ErrorAction SilentlyContinue

# Build with auto-download (will fetch Firebird 5.0.2)
.\build-windows.ps1 -Arch arm64 -Clean -FirebirdVersion "5.0.2"
```

### Option 2: Manual Installation

1. **Download Firebird 5.0.2**:
   - URL: https://firebirdsql.org/en/firebird-5-0/
   - Direct: https://github.com/FirebirdSQL/firebird/releases/download/v5.0.2/Firebird-5.0.2-windows-x64.zip

2. **Extract to location**:
   ```powershell
   # Extract to convenient location
   Expand-Archive -Path Firebird-5.0.2-windows-x64.zip -DestinationPath "C:\Firebird" -Force
   ```

3. **Build with manual path**:
   ```powershell
   .\build-windows.ps1 -Arch arm64 -FirebirdRoot "C:\Firebird\Firebird-5.0.2.0" -Clean
   ```

## Build Commands

### Full Automated Build
```powershell
# Clean build with Firebird 5.0.2 auto-download
.\build-windows.ps1 -Arch arm64 -Clean -FirebirdVersion "5.0.2"
```

### Manual CMake Build
```powershell
# After installing Firebird 5.0.2
cmake -B build_win_arm64 -A ARM64 `
  -DFIREBIRD_ROOT="C:\Program Files\Firebird\Firebird_5_0" `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build_win_arm64 --config Release
```

## Verification

### Successful Build Output
```
build_win_arm64/
└── plugin/
    └── FirebirdPlugin/
        └── Build Resources/
            └── Windows arm64/
                └── FirebirdPlugin.dll    # Native ARM64 plugin
```

### File Check
```powershell
# Verify ARM64 DLL was created
Get-ChildItem -Recurse build_win_arm64\plugin\*.dll
```

## Troubleshooting

### "Firebird not found"
- Ensure Firebird 5.0.2 is extracted
- Check path includes `\include\ibase.h`
- Try full path: `-FirebirdRoot "C:\Full\Path\To\Firebird"`

### "ARM64 build tools not found"
- Install MSVC ARM64 build tools via Visual Studio Installer
- Individual Components → "MSVC ARM64 build tools"

### API compilation errors
- Verify you're using Firebird 5.0.2 (not 6.0)
- Clean build directory: `-Clean` parameter
- Check CMake cache was removed

## Next Steps After Success

1. **Install plugin in Xojo**:
   ```powershell
   # Copy to Xojo plugins folder
   Copy-Item -Recurse build_win_arm64\plugin\FirebirdPlugin `
     "C:\Program Files\Xojo\Xojo 2025 Release X.X\Plugins\"
   ```

2. **Test in Xojo IDE**:
   - Create new Xojo project
   - Check for `FirebirdDatabase` class
   - Test connection to Firebird database

3. **Commit successful build**:
   ```bash
   git add -A
   git commit -m "Windows ARM64 build working with Firebird 5.0.2"
   ```

## Platform Support Matrix

| Platform | Firebird Version | Status |
|----------|------------------|---------|
| Windows ARM64 | Firebird 5.0.2 | ✅ Target |
| Windows x64 | Firebird 5.0.2 | ✅ Compatible |
| macOS ARM64 | Firebird 5.x | ✅ Existing |
| Linux x64 | Firebird 5.x | ✅ Existing |
