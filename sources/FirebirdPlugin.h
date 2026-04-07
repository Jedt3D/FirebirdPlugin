// FirebirdPlugin.h — Xojo Plugin bridge for Firebird SQL
// Defines the custom dbDatabase, dbCursor structs and the class data
// that ties everything to the Xojo runtime.

#ifndef FIREBIRD_PLUGIN_H
#define FIREBIRD_PLUGIN_H

// Windows requires WinHeader++.h before any other SDK include
#if defined(_WIN32) || defined(WIN32)
    #include "WinHeader++.h"
#endif

#include "FirebirdDB.h"
#include "rb_plugin.h"

// ---------------------------------------------------------------------------
// Plugin-side database struct — stored inside the Xojo Database object
// ---------------------------------------------------------------------------
struct FirebirdDbData {
    FBDatabase *db;
    bool        autoCommit;     // when true, commit after each ExecuteSQL
};

// ---------------------------------------------------------------------------
// Plugin-side cursor struct — stored per RowSet
// ---------------------------------------------------------------------------
struct FirebirdCursorData {
    FBDatabase    *db;          // non-owning pointer to the parent database
    FBStatement   *stmt;        // owns this statement
    bool           firstRowCalled;
    bool           eof;
    bool           bof;
    bool           allRowsFetched;
    int            currentRow;
    std::vector<std::vector<FBValue>> rows;
    void          *lastValue;   // last allocated value, freed on next call or release
    unsigned char  lastType;    // dbFieldType of last value
};

// ---------------------------------------------------------------------------
// Plugin-side prepared statement class data
// ---------------------------------------------------------------------------
struct FirebirdPreparedStmtData {
    FBDatabase  *db;            // non-owning
    FBStatement *stmt;          // owns this statement
};

// ---------------------------------------------------------------------------
// Class instance data for FirebirdDatabase (Xojo class)
// ---------------------------------------------------------------------------
struct FirebirdClassData {
    REALstring host;
    REALstring databaseName;
    REALstring userName;
    REALstring password;
    REALstring characterSet;
    REALstring role;
    REALstring wireCrypt;
    REALstring authClientPlugins;
    int32_t    sslMode;
    int32_t    port;
    int32_t    dialect;
};

// Entry point — called by PluginMain.cpp glue code
extern "C" void PluginEntry();

#endif // FIREBIRD_PLUGIN_H
