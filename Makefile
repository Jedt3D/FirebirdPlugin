# FirebirdPlugin for Xojo — macOS Build
# Requires: Xcode CLI tools, Firebird 6.0 installed at /Library/Frameworks/Firebird.framework

PLUGIN_NAME = FirebirdPlugin
SDK_DIR = sdk
SRC_DIR = sources

# Firebird paths
FB_FRAMEWORK = /Library/Frameworks/Firebird.framework
FB_INCLUDE = $(FB_FRAMEWORK)/Versions/A/Headers
FB_LIB = $(FB_FRAMEWORK)/Versions/A/Libraries

# Xojo Plugin SDK paths
SDK_INCLUDES = $(SDK_DIR)/Includes
SDK_GLUE = $(SDK_DIR)/GlueCode

# Compiler settings
CXX = clang++
CXXFLAGS = -std=c++17 -O2 -fPIC -fvisibility=hidden \
           -DCOCOA=1 -DTARGET_COCOA=1 -DTARGET_CARBON=1 -DTARGET_MACOS=1 \
           -I$(SDK_INCLUDES) -I$(FB_INCLUDE) \
           -Wno-deprecated-declarations -Wno-nullability-completeness

OBJCFLAGS = $(CXXFLAGS) -fobjc-arc

LDFLAGS = -dynamiclib -arch arm64 \
          -framework Carbon -framework Cocoa -framework CoreFoundation \
          -L$(FB_LIB) -lfbclient \
          -exported_symbols_list exports.txt \
          -install_name @rpath/$(PLUGIN_NAME).dylib

# For bundled mode: rewrite libfbclient path so the app looks next to itself
# instead of requiring /Library/Frameworks/Firebird.framework
BUNDLED_LDFLAGS = -dynamiclib -arch arm64 \
          -framework Carbon -framework Cocoa -framework CoreFoundation \
          -L$(FB_LIB) -lfbclient \
          -exported_symbols_list exports.txt \
          -install_name @rpath/$(PLUGIN_NAME).dylib

# Source files
SOURCES = $(SRC_DIR)/FirebirdDB.cpp \
          $(SRC_DIR)/FirebirdPlugin.cpp \
          $(SDK_GLUE)/PluginMain.cpp

OBJC_SOURCES = $(SDK_GLUE)/PluginMainCocoa.mm

OBJECTS = $(SOURCES:.cpp=.o) $(OBJC_SOURCES:.mm=.o)

# Output
OUTPUT_DIR = build
DYLIB = $(OUTPUT_DIR)/$(PLUGIN_NAME).dylib

.PHONY: all clean plugin

all: $(DYLIB)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(DYLIB): $(OBJECTS) exports.txt | $(OUTPUT_DIR)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "Built: $@"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.mm
	$(CXX) $(OBJCFLAGS) -c $< -o $@

exports.txt:
	echo "_REALPluginMain" > exports.txt

# Package as .xojo_plugin (ZIP archive — required by Xojo)
PLUGIN_STAGING = $(OUTPUT_DIR)/staging
plugin: $(DYLIB)
	rm -rf "$(PLUGIN_STAGING)" "$(OUTPUT_DIR)/$(PLUGIN_NAME).xojo_plugin"
	mkdir -p "$(PLUGIN_STAGING)/$(PLUGIN_NAME)/Build Resources/Mac arm64"
	cp $(DYLIB) "$(PLUGIN_STAGING)/$(PLUGIN_NAME)/Build Resources/Mac arm64/$(PLUGIN_NAME).dylib"
	touch "$(PLUGIN_STAGING)/Version.info"
	cd "$(PLUGIN_STAGING)" && zip -r "../$(PLUGIN_NAME).xojo_plugin" . -x ".*"
	rm -rf "$(PLUGIN_STAGING)"
	@echo "Plugin packaged: $(OUTPUT_DIR)/$(PLUGIN_NAME).xojo_plugin"

# Build variant that looks for libfbclient next to the app (via @rpath)
bundled: $(OBJECTS) exports.txt | $(OUTPUT_DIR)
	$(CXX) $(BUNDLED_LDFLAGS) -o $(DYLIB) $(OBJECTS)
	install_name_tool -change \
		"/Library/Frameworks/Firebird.framework/Libraries/libfbclient.dylib" \
		"@executable_path/../Frameworks/libfbclient.dylib" \
		$(DYLIB)
	@echo "Built (bundled mode): $(DYLIB)"
	@echo "Users must place libfbclient.dylib in AppName.app/Contents/Frameworks/"

# Install to Xojo plugins folder
install: plugin
	cp "$(OUTPUT_DIR)/$(PLUGIN_NAME).xojo_plugin" "/Applications/Xojo 2025 Release 3.1/Plugins/"
	@echo "Installed to Xojo Plugins folder"

clean:
	rm -rf $(OUTPUT_DIR) exports.txt
	find . -name "*.o" -delete
