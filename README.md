# FirebirdPlugin for Xojo

A native Xojo plugin that adds **FirebirdSQL** database support to Xojo, integrating with the built-in `Database` class hierarchy (RowSet, PreparedStatement, etc.) just like the MySQL and PostgreSQL plugins that ship with Xojo.

Built on the **Firebird legacy C API** (`ibase.h` / `libfbclient`) and the **Xojo Plugin SDK**.

## Features

- **FirebirdDatabase** class — inherits from `Database`, works with Xojo API 2.0 patterns
- `SelectSQL` / `ExecuteSQL` returning `RowSet`
- `Prepare` returning `PreparedStatement` with typed `Bind` methods
- Transactions: `BeginTransaction`, `CommitTransaction`, `RollbackTransaction`
- Schema introspection: `TableSchema`, `FieldSchema`, `IndexSchema`
- Firebird-specific properties: `Host`, `Port`, `DatabaseName`, `CharacterSet`, `Role`, `Dialect`
- Full type mapping: INTEGER, BIGINT, FLOAT, DOUBLE, NUMERIC/DECIMAL, VARCHAR, CHAR, BLOB (text & binary), DATE, TIME, TIMESTAMP, BOOLEAN
- Supports **remote** (client/server) and **embedded** (direct file) connections
- Cross-platform: **macOS** (arm64, x86_64), **Windows** (x64), **Linux** (x64)

## Quick Example (Xojo API 2.0)

```vb
Var db As New FirebirdDatabase

db.Host = "localhost"
db.Port = 3050
db.DatabaseName = "/var/firebird/data/myapp.fdb"
db.UserName = "SYSDBA"
db.Password = "masterkey"
db.CharacterSet = "UTF8"

Try
  db.Connect

  // Create a table
  db.ExecuteSQL("CREATE TABLE users (id INTEGER PRIMARY KEY, name VARCHAR(100))")

  // Insert with parameters
  db.ExecuteSQL("INSERT INTO users (id, name) VALUES (?, ?)", 1, "Alice")

  // Query
  Var rs As RowSet = db.SelectSQL("SELECT * FROM users WHERE id = ?", 1)
  While Not rs.AfterLastRow
    System.DebugLog(rs.Column("name").StringValue)
    rs.MoveToNextRow
  Wend
  rs.Close

  db.Close
Catch err As DatabaseException
  System.DebugLog("Error: " + err.Message)
End Try
```

More examples in the [`examples/`](examples/) directory, including prepared statements, transactions, NULL handling, date/time types, BLOBs, and Firebird-specific features (RETURNING clauses, CTEs, window functions, EXECUTE BLOCK).

## Project Structure

```
FirebirdPlugin/
├── sources/
│   ├── FirebirdDB.h          # C++ wrapper over libfbclient (isc_* API)
│   ├── FirebirdDB.cpp         # Connection, transactions, statements, BLOB I/O
│   ├── FirebirdPlugin.h       # Plugin bridge structs (engine, cursor, class data)
│   └── FirebirdPlugin.cpp     # Xojo Plugin SDK integration & PluginEntry()
├── sdk/
│   ├── Includes/              # Xojo Plugin SDK headers (rb_plugin.h, etc.)
│   └── GlueCode/             # PluginMain.cpp, PluginMainCocoa.mm
├── examples/
│   ├── FirebirdExample_API2.xojo_code   # 15 usage examples
│   └── QuickReference.xojo_code         # Side-by-side comparison with MySQL/PostgreSQL
├── .github/workflows/
│   └── build.yml              # GitHub Actions CI for all platforms
├── CMakeLists.txt             # Cross-platform build (macOS/Windows/Linux)
├── Makefile                   # macOS-only convenience build
├── build-windows.ps1          # Windows build script (auto-downloads Firebird)
└── README.md
```

## Architecture

The plugin has two layers:

1. **`FirebirdDB` (C++ layer)** — A thin wrapper over Firebird's legacy C API. `FBDatabase` manages connections and transactions; `FBStatement` manages prepared statements with XSQLDA descriptors. This layer handles DPB construction, buffer allocation, type-aware value extraction, and BLOB I/O. The Xojo plugin bridge never touches `isc_*` functions directly.

2. **`FirebirdPlugin` (SDK bridge layer)** — Implements the Xojo Plugin SDK interfaces: `REALdbEngineDefinition` (database operations), `REALdbCursorDefinition` (RowSet navigation), and `REALclassDefinition` (the FirebirdDatabase class visible to Xojo code). `PluginEntry()` registers everything with the Xojo runtime.

## Prerequisites

### Firebird Client Library

The plugin links dynamically against **libfbclient** at runtime. You need the Firebird client installed on the machine where your built Xojo app runs.

| Platform | Install | What gets installed |
|----------|---------|-------------------|
| **macOS** | [Download .pkg](https://firebirdsql.org/en/firebird-5-0/) or `brew install firebird` | `/Library/Frameworks/Firebird.framework/` (headers + `libfbclient.dylib`) |
| **Windows** | [Download installer](https://firebirdsql.org/en/firebird-5-0/) — choose "Client only" if you don't need the server | `fbclient.dll` + headers in install directory |
| **Linux** | `sudo apt-get install firebird-dev libfbclient2` (Debian/Ubuntu) or download from [firebirdsql.org](https://firebirdsql.org/en/firebird-5-0/) | `/usr/include/firebird/ibase.h` + `/usr/lib/libfbclient.so` |

> **End-user machines** only need the client library (`libfbclient.dylib` / `fbclient.dll` / `libfbclient.so`), not the full Firebird server — unless they also run a local Firebird database.

### Xojo Plugin SDK

The `sdk/` directory includes the required Plugin SDK headers and glue code from the Xojo Extras. These are the files from `Xojo/Extras/PluginsSDK/` that ship with Xojo.

### Build Tools

- **CMake** 3.20+ (cross-platform builds)
- **macOS**: Xcode Command Line Tools (`xcode-select --install`)
- **Windows**: Visual Studio 2022 with C++ workload (for ARM64: add "MSVC v143 ARM64 build tools" in VS Installer)
- **Linux**: GCC/G++ and standard build tools (`build-essential`)

## Building

### Windows (PowerShell build script)

The easiest way to build on Windows. The script auto-detects VS2022, downloads Firebird, and runs CMake:

```powershell
# Build for x64 (default) — downloads Firebird automatically
.\build-windows.ps1

# Build for ARM64
.\build-windows.ps1 -Arch arm64

# Clean rebuild
.\build-windows.ps1 -Clean

# Use an existing Firebird installation
.\build-windows.ps1 -FirebirdRoot "C:\Program Files\Firebird\Firebird_5_0"

# Skip download (reuse previous)
.\build-windows.ps1 -SkipFirebird
```

> **ARM64 note**: Firebird does not yet ship native ARM64 Windows builds. The script downloads the x64 client, which works under Windows ARM64's x64 emulation layer. The plugin DLL itself will be native ARM64.

### macOS (Makefile — quick local build)

```bash
# Requires Firebird.framework installed at /Library/Frameworks/
make            # builds FirebirdPlugin.dylib
make plugin     # packages as FirebirdPlugin.xojo_plugin (ZIP)
make install    # copies to Xojo Plugins folder
```

### Cross-platform (CMake — manual)

```bash
# macOS (arm64)
cmake -B build -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# macOS (x86_64)
cmake -B build -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Windows x64 (from Developer Command Prompt or PowerShell)
cmake -B build -G "Visual Studio 17 2022" -A x64 -DFIREBIRD_ROOT="C:\Firebird" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Windows ARM64
cmake -B build -G "Visual Studio 17 2022" -A ARM64 -DFIREBIRD_ROOT="C:\Firebird" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Linux
cmake -B build -DCMAKE_BUILD_TYPE=Release -DFIREBIRD_ROOT=/usr
cmake --build build --config Release
```

The built binary lands in `build/plugin/FirebirdPlugin/Build Resources/<platform>/`.

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `FIREBIRD_ROOT` | Auto-detect | Path to Firebird installation (Windows/Linux) |
| `CMAKE_OSX_ARCHITECTURES` | Host arch | `arm64`, `x86_64`, or `arm64;x86_64` for universal |
| `CMAKE_BUILD_TYPE` | — | `Release` recommended for plugin builds |

## Installing the Plugin

1. Build or download `FirebirdPlugin.xojo_plugin` (a ZIP archive)
2. Copy it to your Xojo Plugins folder:
   - **macOS**: `/Applications/Xojo 2025 Release X.X/Plugins/`
   - **Windows**: `C:\Program Files\Xojo\Xojo 2025 Release X.X\Plugins\`
   - **Linux**: `/opt/xojo/Plugins/` or wherever Xojo is installed
3. Restart Xojo — `FirebirdDatabase` will appear in autocomplete

## CI / GitHub Actions

The included workflow (`.github/workflows/build.yml`) builds for all platforms in parallel:

- **macOS**: arm64 + x86_64 (separate artifacts)
- **Windows**: x64 (static CRT, no vcruntime dependency)
- **Linux**: x64

The `package` job merges all platform binaries into a single `.xojo_plugin` ZIP. When you push a version tag (`v*`), it automatically creates a GitHub Release with the plugin attached.

```bash
git tag v0.1.0
git push origin v0.1.0
# → GitHub Actions builds all platforms and publishes a release
```

## Plugin Packaging Format

Xojo expects `.xojo_plugin` files to be **ZIP archives** with this structure:

```
Version.info                                          # (can be empty)
FirebirdPlugin/
  Build Resources/
    Mac arm64/FirebirdPlugin.dylib
    Mac x86-64/FirebirdPlugin.dylib
    Windows x86-64/FirebirdPlugin.dll
    Linux x86-64/FirebirdPlugin.so
```

## Firebird SQL Notes for Xojo Developers

If you're coming from MySQL or PostgreSQL, here are some Firebird differences:

| Feature | Firebird | MySQL/PostgreSQL |
|---------|----------|-----------------|
| Auto-increment | `GENERATED BY DEFAULT AS IDENTITY` | `AUTO_INCREMENT` / `SERIAL` |
| String concat | `\|\|` | `CONCAT()` or `+` |
| Boolean | Native `BOOLEAN` type | Varies |
| LIMIT | `FIRST n SKIP m` or `FETCH FIRST n ROWS ONLY` | `LIMIT n OFFSET m` |
| Case sensitivity | Unquoted identifiers are UPPERCASE | Varies |
| Default port | 3050 | 3306 / 5432 |

## Acknowledgments

- **[CubeSQL Plugin](https://github.com/marcobambini/cubesqlplugin)** by Marco Bambini — An invaluable open-source reference for implementing a Xojo database plugin using the Plugin SDK. The CubeSQL plugin's architecture, CI pipeline, and approach to bridging `REALdbEngineDefinition` / `REALdbCursorDefinition` informed much of this plugin's design. If you're building a Xojo database plugin, CubeSQL's source code is the best real-world example available.
- **[Firebird SQL](https://firebirdsql.org/)** — The powerful, lightweight, open-source relational database that this plugin connects to.
- **[Xojo](https://www.xojo.com/)** — The cross-platform development environment and Plugin SDK.

## License

This project is open source. See [LICENSE](LICENSE) for details.

---

*Built with the help of [Claude Code](https://claude.ai/claude-code).*
