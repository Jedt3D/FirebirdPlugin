# Windows ARM64 Build Status

**⚠️ IMPORTANT STATUS UPDATE (2026-04-07)**

## Current Status: Moved to Experimental Branch

Windows ARM64 builds are **no longer supported on the main branch**. ARM64 development has been moved to the dedicated `feature/firebird6-arm64` branch due to technical requirements for Firebird 6.0 modern C++ API integration.

## 🎯 Architecture Strategy

### Main Branch (Production)
✅ **Win32 + x64** with Firebird 5.0.3
- Production-ready builds
- Stable and well-tested
- Covers 95%+ of Windows users

### Feature Branch (Experimental)
🔬 **ARM64** with Firebird 6.0 Beta
- Requires modern C++ API integration
- Complex Windows compatibility challenges
- **Not production-ready**

## 🔬 For ARM64 Development

If you want to work on Windows ARM64 support:

```bash
# Switch to the experimental branch
git checkout feature/firebird6-arm64

# Work on Firebird 6.0 integration
# Branch is ready for community contribution
```

### Technical Challenges
The `feature/firebird6-arm64` branch faces these challenges:
1. **Modern C++ API**: Firebird 6.0 uses new interfaces not available in Firebird 5.x
2. **Windows Headers**: Different structure than macOS/Linux Firebird builds
3. **API Integration**: Complex compatibility requirements with Windows MSVC
4. **Testing**: Requires ARM64 hardware or emulation for runtime testing

## 🚀 For Production Deployment

### Recommended Approach for ARM64 Users

**Option 1: Use x64 Plugin (Recommended)**
```powershell
# Use the x64 build on ARM64 Windows
# It runs under WOW64 emulation with excellent performance
build_x64_fb5/Release/FirebirdPlugin.dll
```

**Option 2: Use macOS ARM64 Build**
- If developing on Mac, use the native macOS ARM64 build
- Full feature parity with Windows version

**Option 3: Contribute to ARM64 Windows Support**
- Join development on `feature/firebird6-arm64` branch
- Help solve Firebird 6.0 integration challenges
- Test on ARM64 hardware

## 📋 Historical Context

### Why This Change?
When the feature-rich code from macOS ARM64 was integrated, it brought dependencies on Firebird's modern C++ API. This API is only available in Firebird 6.0, and:
- **Firebird 5.x**: No ARM64 Windows build exists
- **Firebird 6.0**: Has ARM64 support but requires modern C++ API
- **Windows Firebird 6.0**: Different header structure than macOS/Linux

### Why Separate Branches?
1. **Main Branch Stability**: Keep Win32/x64 production-ready
2. **Focused Development**: ARM64 work doesn't destabilize main branch
3. **Clear Expectations**: Users know what's production-ready
4. **Community Contribution**: Easy for contributors to join ARM64 effort

## 🔧 Build Commands (For Reference)

### Main Branch - Production Builds
```powershell
# Win32 (x86)
cmake -B build_win32_fb5 -G "Visual Studio 18 2026" -A Win32 `
  -DFIREBIRD_ROOT="C:/Users/worajedt/Firebird-5.0.3.1683-0-windows-x86-withDebugSymbols"
cmake --build build_win32_fb5 --config Release

# x64
cmake -B build_x64_fb5 -G "Visual Studio 18 2026" -A x64 `
  -DFIREBIRD_ROOT="C:/Users/worajedt/Firebird-5.0.3.1683-0-windows-x64-withDebugSymbols"
cmake --build build_x64_fb5 --config Release
```

### Feature Branch - Experimental ARM64
```powershell
# Switch to experimental branch first
git checkout feature/firebird6-arm64

# ARM64 (Experimental - not working yet)
cmake -B build_arm64_fb6 -G "Visual Studio 18 2026" -A ARM64 `
  -DFIREBIRD_ROOT="C:/Program Files/Firebird/Firebird_6_0"
cmake --build build_arm64_fb6 --config Release
```

## 💡 Recommendations

### For End Users
- **Use Win32/x64 builds** from main branch
- **If on ARM64 Windows**: Use x64 plugin via WOW64 emulation
- **Monitor `feature/firebird6-arm64`** branch for progress

### For Developers
- **Main branch**: Focus on Win32/x64 stability and features
- **Feature branch**: Work on ARM64 + Firebird 6.0 integration
- **Testing**: Contribute testing on ARM64 hardware

### For Project Planning
- **Short-term**: Win32/x64 with Firebird 5.0.3
- **Medium-term**: Monitor Firebird 6.0 stability
- **Long-term**: Native ARM64 Windows support when Firebird 6.0 matures

## 📊 Status Matrix

| Platform | Architecture | Branch | Firebird | Status | Build Command |
|----------|-------------|---------|----------|---------|---------------|
| Windows | Win32 (x86) | `main` | FB 5.0.3 | ✅ Production | `build_win32_fb5` |
| Windows | x64 | `main` | FB 5.0.3 | ✅ Production | `build_x64_fb5` |
| Windows | ARM64 | `feature/firebird6-arm64` | FB 6.0 | 🔬 Experimental | Not working |
| macOS | ARM64 | Feature branches | FB 6.0 | ✅ Working | See branches |
| macOS | x86-64 | Feature branches | FB 5.x/6.x | ✅ Working | See branches |
| Linux | x86-64 | Feature branches | FB 5.x/6.x | ✅ Working | See branches |

## 🎯 Future Outlook

### When Will ARM64 Windows Be Ready?
The timeline depends on:
1. **Firebird 6.0 Stability**: When it exits beta
2. **API Integration**: Solving modern C++ API compatibility
3. **Community Contribution**: Developer interest and effort
4. **Testing Resources**: Access to ARM64 hardware

### How to Help
- ⭐ **Star the `feature/firebird6-arm64` branch**
- 🤝 **Submit pull requests** for Firebird 6.0 integration
- 🐛 **Test on ARM64 hardware** and report issues
- 💡 **Share expertise** on modern C++ API integration
- 📖 **Improve documentation** for ARM64 builds

## 📞 Resources

### Current Documentation
- [BUILD_STATUS.md](BUILD_STATUS.md) - Overall build status
- [README.md](README.md) - Project overview
- [Feature Branch](https://github.com/Jedt3D/FirebirdPlugin/tree/feature/firebird6-arm64) - ARM64 development

### Firebird Resources
- [Firebird 5.0.3 (Stable)](https://firebirdsql.org/en/firebird-5-0/)
- [Firebird 6.0 Beta](https://firebirdsql.org/en/firebird-6-0/)
- [Firebird Documentation](https://firebirdsql.org/en/documentation/)

---

**Summary**: Windows ARM64 support is under active development on the `feature/firebird6-arm64` branch. Use Win32/x64 builds from main branch for production deployments.
