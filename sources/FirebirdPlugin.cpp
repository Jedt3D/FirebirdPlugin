// FirebirdPlugin.cpp — Xojo Plugin bridge for Firebird SQL
// Implements REALdbEngineDefinition, REALdbCursorDefinition,
// FirebirdDatabase class, and PluginEntry().

#include "FirebirdPlugin.h"
#include <cstring>
#include <string>
#include <sstream>
#include <ctime>

// ============================================================================
// Forward declarations — all callback functions
// ============================================================================

// DB Engine callbacks
static void         fbEngineClose(dbDatabase *);
static REALdbCursor fbEngineTableSchema(dbDatabase *);
static REALdbCursor fbEngineFieldSchema(dbDatabase *, REALstring table);
static REALdbCursor fbEngineDirectSQLSelect(dbDatabase *, REALstring sql);
static void         fbEngineDirectSQLExecute(dbDatabase *, REALstring sql);
static REALdbCursor fbEngineSelectSQL(dbDatabase *, REALstring sql, REALarray params);
static void         fbEngineExecuteSQL(dbDatabase *, REALstring sql, REALarray params);
static long         fbEngineGetLastErrorCode(dbDatabase *);
static REALstring   fbEngineGetLastErrorString(dbDatabase *);
static void         fbEngineCommit(dbDatabase *);
static void         fbEngineRollback(dbDatabase *);
static void         fbEngineBeginTransaction(dbDatabase *);
static REALobject   fbEnginePrepareStatement(dbDatabase *, REALstring sql);
static REALdbCursor fbEngineGetDatabaseIndexes(dbDatabase *, REALstring table);

// Cursor callbacks
static void         fbCursorClose(dbCursor *);
static int          fbCursorColumnCount(dbCursor *);
static REALstring   fbCursorColumnName(dbCursor *, int col);
static int          fbCursorRowCount(dbCursor *);
static void         fbCursorColumnValue(dbCursor *, int col, void **value, unsigned char *type, int *length);
static void         fbCursorReleaseValue(dbCursor *);
static RBBoolean    fbCursorNextRow(dbCursor *);
static int          fbCursorColumnType(dbCursor *, int index);
static RBBoolean    fbCursorIsBOF(dbCursor *);
static RBBoolean    fbCursorIsEOF(dbCursor *);

// FirebirdDatabase class methods
static void         fbClassConstructor(REALobject instance);
static void         fbClassDestructor(REALobject instance);
static RBBoolean    fbClassConnect(REALobject instance);
static REALstring   fbClassServerVersion(REALobject instance);
static RBInteger    fbClassPageSize(REALobject instance);
static RBInteger    fbClassDatabaseSQLDialect(REALobject instance);
static REALstring   fbClassODSVersion(REALobject instance);
static RBBoolean    fbClassIsReadOnly(REALobject instance);
static RBBoolean    fbClassHasActiveTransaction(REALobject instance);
static RBInt64      fbClassTransactionID(REALobject instance);
static REALstring   fbClassTransactionIsolation(REALobject instance);
static REALstring   fbClassTransactionAccessMode(REALobject instance);
static RBInteger    fbClassTransactionLockTimeout(REALobject instance);
static RBBoolean    fbClassBeginTransactionWithOptions(REALobject instance, REALstring isolation, RBBoolean readOnly, RBInteger lockTimeout);

// Prepared statement class methods
static void         fbPrepStmtConstructor(REALobject instance);
static void         fbPrepStmtDestructor(REALobject instance);
static void         fbPrepStmtBindString(REALobject instance, int32_t index, REALstring value);
static void         fbPrepStmtBindInt(REALobject instance, int32_t index, RBInt64 value);
static void         fbPrepStmtBindDouble(REALobject instance, int32_t index, double value);
static void         fbPrepStmtBindBoolean(REALobject instance, int32_t index, RBBoolean value);
static void         fbPrepStmtBindDateTime(REALobject instance, int32_t index, REALobject value);
static void         fbPrepStmtBindTextBlob(REALobject instance, int32_t index, REALstring value);
static void         fbPrepStmtBindBinaryBlob(REALobject instance, int32_t index, REALobject value);
static void         fbPrepStmtBindNull(REALobject instance, int32_t index);
static REALdbCursor fbPrepStmtSelectSQL(REALobject instance);
static void         fbPrepStmtExecuteSQL(REALobject instance);

// ============================================================================
// Static data structures (must be defined before functions that reference them)
// ============================================================================

// --- DB Engine Definition ---------------------------------------------------

static REALdbEngineDefinition sFirebirdEngine = {
    kCurrentREALDatabaseVersion,
    0,                                       // forSystemUse
    (unsigned char)(dbEnginePrimaryKeySupported | dbEngineDontUseBrackets),
    0, 0,                                    // flags2, flags3
    fbEngineClose,
    fbEngineTableSchema,
    fbEngineFieldSchema,
    fbEngineDirectSQLSelect,
    fbEngineDirectSQLExecute,
    nullptr,                                 // createTable
    nullptr,                                 // addTableRecord
    fbEngineSelectSQL,
    nullptr,                                 // updateFields
    nullptr,                                 // addTableColumn
    fbEngineGetDatabaseIndexes,
    fbEngineGetLastErrorCode,
    fbEngineGetLastErrorString,
    fbEngineCommit,
    fbEngineRollback,
    fbEngineBeginTransaction,
    nullptr,                                 // getSupportedTypes
    nullptr,                                 // dropTable
    nullptr,                                 // dropColumn
    nullptr,                                 // alterTableName
    nullptr,                                 // alterColumnName
    nullptr,                                 // alterColumnType
    nullptr,                                 // alterColumnConstraint
    fbEnginePrepareStatement,
    fbEngineExecuteSQL,
    nullptr,                                 // AddRowWithReturnValue
};

// --- DB Cursor Definition ---------------------------------------------------

static REALdbCursorDefinition sFirebirdCursor = {
    kCurrentREALDatabaseVersion,
    0,                                       // forSystemUse
    fbCursorClose,
    fbCursorColumnCount,
    fbCursorColumnName,
    fbCursorRowCount,
    fbCursorColumnValue,
    fbCursorReleaseValue,
    fbCursorNextRow,
    nullptr,                                 // cursorDelete
    nullptr,                                 // cursorDeleteAll
    nullptr,                                 // cursorFieldKey
    nullptr,                                 // cursorUpdate
    nullptr,                                 // cursorEdit
    nullptr,                                 // cursorPrevRow
    nullptr,                                 // cursorFirstRow
    nullptr,                                 // cursorLastRow
    fbCursorColumnType,
    fbCursorIsBOF,
    fbCursorIsEOF,
    nullptr,                                 // dummy10
};

// --- FirebirdDatabase Class -------------------------------------------------

static REALproperty sFirebirdClassProperties[] = {
    { "", "Host", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, host) },
    { "", "Port", "Integer", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, port) },
    { "", "DatabaseName", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, databaseName) },
    { "", "UserName", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, userName) },
    { "", "Password", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, password) },
    { "", "CharacterSet", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, characterSet) },
    { "", "Role", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, role) },
    { "", "Dialect", "Integer", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, dialect) },
};

static REALmethodDefinition sFirebirdClassMethods[] = {
    { (REALproc)fbClassConnect, REALnoImplementation, "Connect() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassServerVersion, REALnoImplementation, "ServerVersion() As String", REALconsoleSafe },
    { (REALproc)fbClassPageSize, REALnoImplementation, "PageSize() As Integer", REALconsoleSafe },
    { (REALproc)fbClassDatabaseSQLDialect, REALnoImplementation, "DatabaseSQLDialect() As Integer", REALconsoleSafe },
    { (REALproc)fbClassODSVersion, REALnoImplementation, "ODSVersion() As String", REALconsoleSafe },
    { (REALproc)fbClassIsReadOnly, REALnoImplementation, "IsReadOnly() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassHasActiveTransaction, REALnoImplementation, "HasActiveTransaction() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassTransactionID, REALnoImplementation, "TransactionID() As Int64", REALconsoleSafe },
    { (REALproc)fbClassTransactionIsolation, REALnoImplementation, "TransactionIsolation() As String", REALconsoleSafe },
    { (REALproc)fbClassTransactionAccessMode, REALnoImplementation, "TransactionAccessMode() As String", REALconsoleSafe },
    { (REALproc)fbClassTransactionLockTimeout, REALnoImplementation, "TransactionLockTimeout() As Integer", REALconsoleSafe },
    { (REALproc)fbClassBeginTransactionWithOptions, REALnoImplementation, "BeginTransactionWithOptions(isolation As String, readOnly As Boolean, lockTimeout As Integer) As Boolean", REALconsoleSafe },
};

static REALclassDefinition sFirebirdDatabaseClass = {
    kCurrentREALControlVersion,
    "FirebirdDatabase",                      // name
    "Database",                              // superName
    sizeof(FirebirdClassData),               // dataSize
    0,                                       // forSystemUse
    (REALproc)fbClassConstructor,
    (REALproc)fbClassDestructor,
    sFirebirdClassProperties,
    sizeof(sFirebirdClassProperties) / sizeof(REALproperty),
    sFirebirdClassMethods,
    sizeof(sFirebirdClassMethods) / sizeof(REALmethodDefinition),
    nullptr, 0,                              // events
    nullptr, 0,                              // eventInstances
    nullptr,                                 // interfaces
    nullptr, 0,                              // attributes
    nullptr, 0,                              // constants
    0,                                       // mFlags
    nullptr, 0,                              // sharedProperties
    nullptr, 0,                              // sharedMethods
#if kCurrentREALControlVersion >= 11
    nullptr, 0,                              // delegates
    nullptr, 0,                              // enums
#endif
};

// --- FirebirdPreparedStatement Class ----------------------------------------

static REALmethodDefinition sFirebirdPreparedStmtMethods[] = {
    { (REALproc)fbPrepStmtBindString, REALnoImplementation,
      "Bind(index As Integer, value As String)", REALconsoleSafe },
    { (REALproc)fbPrepStmtBindInt, REALnoImplementation,
      "Bind(index As Integer, value As Int64)", REALconsoleSafe },
    { (REALproc)fbPrepStmtBindDouble, REALnoImplementation,
      "Bind(index As Integer, value As Double)", REALconsoleSafe },
    { (REALproc)fbPrepStmtBindBoolean, REALnoImplementation,
      "Bind(index As Integer, value As Boolean)", REALconsoleSafe },
    { (REALproc)fbPrepStmtBindDateTime, REALnoImplementation,
      "Bind(index As Integer, value As DateTime)", REALconsoleSafe },
    { (REALproc)fbPrepStmtBindTextBlob, REALnoImplementation,
      "BindTextBlob(index As Integer, value As String)", REALconsoleSafe },
    { (REALproc)fbPrepStmtBindBinaryBlob, REALnoImplementation,
      "BindBinaryBlob(index As Integer, value As MemoryBlock)", REALconsoleSafe },
    { (REALproc)fbPrepStmtBindNull, REALnoImplementation,
      "BindNull(index As Integer)", REALconsoleSafe },
    { (REALproc)fbPrepStmtSelectSQL, REALnoImplementation,
      "SelectSQL() As RowSet", REALconsoleSafe },
    { (REALproc)fbPrepStmtExecuteSQL, REALnoImplementation,
      "ExecuteSQL()", REALconsoleSafe },
};

static REALclassDefinition sFirebirdPreparedStmtClass = {
    kCurrentREALControlVersion,
    "FirebirdPreparedStatement",             // name
    nullptr,                                  // superName (none — framework routes via engine callback)
    sizeof(FirebirdPreparedStmtData),        // dataSize
    0,                                       // forSystemUse
    (REALproc)fbPrepStmtConstructor,
    (REALproc)fbPrepStmtDestructor,
    nullptr, 0,                              // properties
    sFirebirdPreparedStmtMethods,
    sizeof(sFirebirdPreparedStmtMethods) / sizeof(REALmethodDefinition),
    nullptr, 0,                              // events
    nullptr, 0,                              // eventInstances
    nullptr,                                 // interfaces
    nullptr, 0,                              // attributes
    nullptr, 0,                              // constants
    0,                                       // mFlags
    nullptr, 0,                              // sharedProperties
    nullptr, 0,                              // sharedMethods
#if kCurrentREALControlVersion >= 11
    nullptr, 0,                              // delegates
    nullptr, 0,                              // enums
#endif
};

// ============================================================================
// Helpers
// ============================================================================

static std::string RealToStd(REALstring rs) {
    if (!rs) return "";
    REALstringData data;
    if (REALGetStringData(rs, kREALTextEncodingUTF8, &data)) {
        std::string result((const char *)data.data, data.length);
        REALDisposeStringData(&data);
        return result;
    }
    return "";
}

static REALstring StdToReal(const std::string &s) {
    return REALBuildStringWithEncoding(s.c_str(), (int)s.size(), kREALTextEncodingUTF8);
}

static FirebirdDbData *GetFirebirdDbData(REALobject instance) {
    if (!instance) return nullptr;
    REALdbDatabase realDb = (REALdbDatabase)instance;
    return (FirebirdDbData *)REALGetDBFromREALdbDatabase(realDb);
}

static short ToXojoDbShort(short value) {
    unsigned short v = (unsigned short)value;
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    v = (unsigned short)((v << 8) | (v >> 8));
#endif
    return (short)v;
}

static dbFieldType FirebirdTypeToXojo(short fbType, short scale, short subtype) {
    short dtype = fbType & ~1;
    switch (dtype) {
        case SQL_TEXT:      return dbTypeChar;
        case SQL_VARYING:   return dbTypeText;
        case SQL_SHORT:
            if (scale < 0) return dbTypeDecimal;
            return dbTypeShort;
        case SQL_LONG:
            if (scale < 0) return dbTypeDecimal;
            return dbTypeLong;
        case SQL_INT64:
            if (scale < 0) return dbTypeDecimal;
            return dbTypeInt64;
#ifdef SQL_INT128
        case SQL_INT128:    return dbTypeText;
#endif
        case SQL_FLOAT:     return dbTypeFloat;
        case SQL_DOUBLE:    return dbTypeDouble;
#ifdef SQL_DEC16
        case SQL_DEC16:     return dbTypeText;
#endif
#ifdef SQL_DEC34
        case SQL_DEC34:     return dbTypeText;
#endif
        case SQL_TYPE_DATE: return dbTypeDate;
        case SQL_TYPE_TIME: return dbTypeTime;
        case SQL_TIMESTAMP: return dbTypeTimeStamp;
#ifdef SQL_TIME_TZ
        case SQL_TIME_TZ:   return dbTypeText;
#endif
#ifdef SQL_TIME_TZ_EX
        case SQL_TIME_TZ_EX: return dbTypeText;
#endif
#ifdef SQL_TIMESTAMP_TZ
        case SQL_TIMESTAMP_TZ: return dbTypeText;
#endif
#ifdef SQL_TIMESTAMP_TZ_EX
        case SQL_TIMESTAMP_TZ_EX: return dbTypeText;
#endif
        case SQL_BLOB:
            if (subtype == 1) return dbTypeText;
            return dbTypeBinary;
#ifdef SQL_BOOLEAN
        case SQL_BOOLEAN:   return dbTypeBoolean;
#endif
        default:            return dbTypeText;
    }
}

static FirebirdCursorData *NewCursorData(FBDatabase *db, FBStatement *stmt) {
    auto *cd = new FirebirdCursorData;
    cd->db = db;
    cd->stmt = stmt;
    cd->firstRowCalled = false;
    cd->eof = false;
    cd->bof = true;
    cd->lastValue = nullptr;
    cd->lastType = 0;
    return cd;
}

static bool IsMemoryBlockObject(REALobject value) {
    static REALclassRef memoryBlockClass = REALGetClassRef("MemoryBlock");
    return value && memoryBlockClass && REALObjectIsA(value, memoryBlockClass);
}

static bool IsDateTimeObject(REALobject value) {
    static REALclassRef dateTimeClass = REALGetClassRef("DateTime");
    return value && dateTimeClass && REALObjectIsA(value, dateTimeClass);
}

static bool FillTmFromDateTime(REALobject value, struct tm &outTm) {
    if (!IsDateTimeObject(value)) return false;

    RBInteger year = 0;
    RBInteger month = 0;
    RBInteger day = 0;
    RBInteger hour = 0;
    RBInteger minute = 0;
    RBInteger second = 0;

    if (!REALGetPropValueInteger(value, "Year", &year)) return false;
    if (!REALGetPropValueInteger(value, "Month", &month)) return false;
    if (!REALGetPropValueInteger(value, "Day", &day)) return false;
    if (!REALGetPropValueInteger(value, "Hour", &hour)) return false;
    if (!REALGetPropValueInteger(value, "Minute", &minute)) return false;
    if (!REALGetPropValueInteger(value, "Second", &second)) return false;

    memset(&outTm, 0, sizeof(outTm));
    outTm.tm_year = (int)year - 1900;
    outTm.tm_mon = (int)month - 1;
    outTm.tm_mday = (int)day;
    outTm.tm_hour = (int)hour;
    outTm.tm_min = (int)minute;
    outTm.tm_sec = (int)second;
    outTm.tm_isdst = -1;
    return true;
}

static bool BindDateTimeValue(FBStatement *stmt, int index, REALobject value) {
    if (!stmt || !value) return false;

    struct tm tmValue;
    if (!FillTmFromDateTime(value, tmValue)) return false;

    switch (stmt->paramBaseType(index)) {
        case SQL_TYPE_DATE: {
            ISC_DATE sqlDate = 0;
            isc_encode_sql_date(&tmValue, &sqlDate);
            stmt->bindDate(index, sqlDate);
            return true;
        }
        case SQL_TYPE_TIME: {
            ISC_TIME sqlTime = 0;
            isc_encode_sql_time(&tmValue, &sqlTime);
            stmt->bindTime(index, sqlTime);
            return true;
        }
        case SQL_TIMESTAMP: {
            ISC_TIMESTAMP sqlTimestamp = {};
            isc_encode_timestamp(&tmValue, &sqlTimestamp);
            stmt->bindTimestamp(index, sqlTimestamp);
            return true;
        }
        default:
            return false;
    }
}

static bool BindTextBlobValue(FBStatement *stmt, FBDatabase *db, int index, REALstring value) {
    if (!stmt || !db || !value) return false;

    std::string text = RealToStd(value);
    stmt->bindBlob(index, *db, text.data(), text.size());
    return true;
}

static bool BindBinaryBlobValue(FBStatement *stmt, FBDatabase *db, int index, REALobject value) {
    if (!stmt || !db || !IsMemoryBlockObject(value)) return false;

    REALmemoryBlock mem = (REALmemoryBlock)value;
    void *ptr = REALMemoryBlockGetPtr(mem);
    RBInteger size = REALMemoryBlockGetSize(mem);
    if (size < 0) return false;
    if (!ptr && size > 0) return false;

    stmt->bindBlob(index, *db, ptr, (size_t)size);
    return true;
}

static void BindParams(FBStatement *stmt, FBDatabase *db, REALarray params) {
    if (!params || !stmt) return;
    RBInteger count = REALGetArrayUBound(params);
    if (count < 0) return;

    for (RBInteger i = 0; i <= count && i < stmt->paramCount(); i++) {
        // Get the Variant object from the array
        REALobject variant = nullptr;
        REALGetArrayValueObject(params, i, &variant);
        if (!variant) {
            stmt->bindNull((int)i);
            continue;
        }

        // Bind based on what the SQL parameter expects, using Variant's named properties
        short ptype = stmt->paramBaseType((int)i);
        bool bound = false;

        if (ptype == SQL_TYPE_DATE || ptype == SQL_TYPE_TIME || ptype == SQL_TIMESTAMP) {
            REALobject dateTimeValue = nullptr;
            if (REALGetPropValue(variant, "DateTimeValue", &dateTimeValue) && dateTimeValue) {
                bound = BindDateTimeValue(stmt, (int)i, dateTimeValue);
            } else if (REALGetPropValue(variant, "ObjectValue", &dateTimeValue) && dateTimeValue) {
                bound = BindDateTimeValue(stmt, (int)i, dateTimeValue);
            }
        } else if (ptype == SQL_BLOB) {
            REALobject objectValue = nullptr;
            if (REALGetPropValue(variant, "ObjectValue", &objectValue) && objectValue) {
                bound = BindBinaryBlobValue(stmt, db, (int)i, objectValue);
            }

            if (!bound) {
                REALstring strVal = nullptr;
                if (REALGetPropValueString(variant, "StringValue", &strVal) && strVal) {
                    bound = BindTextBlobValue(stmt, db, (int)i, strVal);
                    REALUnlockString(strVal);
                }
            }
        }

        switch (ptype) {
            case SQL_SHORT:
            case SQL_LONG:
            case SQL_INT64: {
                if (bound) break;
                if (stmt->paramScale((int)i) < 0) {
                    double dblVal = 0;
                    if (REALGetPropValueDouble(variant, "DoubleValue", &dblVal)) {
                        stmt->bindDouble((int)i, dblVal);
                        bound = true;
                    }
                } else {
                    RBInt64 intVal = 0;
                    if (REALGetPropValueInt64(variant, "Int64Value", &intVal)) {
                        stmt->bindInt((int)i, intVal);
                        bound = true;
                    }
                }
                break;
            }
            case SQL_FLOAT:
            case SQL_DOUBLE: {
                if (bound) break;
                double dblVal = 0;
                if (REALGetPropValueDouble(variant, "DoubleValue", &dblVal)) {
                    stmt->bindDouble((int)i, dblVal);
                    bound = true;
                }
                break;
            }
#ifdef SQL_BOOLEAN
            case SQL_BOOLEAN: {
                if (bound) break;
                bool boolVal = false;
                if (REALGetPropValueBoolean(variant, "BooleanValue", &boolVal)) {
                    stmt->bindBoolean((int)i, boolVal);
                    bound = true;
                }
                break;
            }
#endif
            default:
                break;
        }

        // Fallback: bind as string using Variant.StringValue
        if (!bound) {
            REALstring strVal = nullptr;
            if (REALGetPropValueString(variant, "StringValue", &strVal) && strVal) {
                stmt->bindString((int)i, RealToStd(strVal));
                REALUnlockString(strVal);
            } else {
                stmt->bindNull((int)i);
            }
        }

        REALUnlockObject(variant);
    }
}

// ============================================================================
// DB Engine Implementation
// ============================================================================

static void fbEngineClose(dbDatabase *dbData) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (fbd) {
        if (fbd->db) {
            fbd->db->disconnect();
            delete fbd->db;
            fbd->db = nullptr;
        }
        delete fbd;
    }
}

static REALdbCursor fbEngineTableSchema(dbDatabase *dbData) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return nullptr;

    auto *stmt = new FBStatement();
    if (!stmt->prepare(*fbd->db, FBDatabase::tableListSQL())) {
        delete stmt;
        return nullptr;
    }
    if (!stmt->execute(*fbd->db)) {
        delete stmt;
        return nullptr;
    }

    auto *cd = NewCursorData(fbd->db, stmt);
    return REALNewRowSetFromDBCursor((dbCursor *)cd, &sFirebirdCursor);
}

static REALdbCursor fbEngineFieldSchema(dbDatabase *dbData, REALstring table) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return nullptr;

    std::string tableName = RealToStd(table);
    for (auto &c : tableName) c = toupper(c);

    auto *stmt = new FBStatement();
    if (!stmt->prepare(*fbd->db, FBDatabase::columnListSQL())) {
        delete stmt;
        return nullptr;
    }
    stmt->bindString(0, tableName);

    if (!stmt->execute(*fbd->db)) {
        delete stmt;
        return nullptr;
    }

    auto *cd = NewCursorData(fbd->db, stmt);
    return REALNewRowSetFromDBCursor((dbCursor *)cd, &sFirebirdCursor);
}

static REALdbCursor fbEngineGetDatabaseIndexes(dbDatabase *dbData, REALstring table) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return nullptr;

    std::string tableName = RealToStd(table);
    for (auto &c : tableName) c = toupper(c);

    auto *stmt = new FBStatement();
    if (!stmt->prepare(*fbd->db, FBDatabase::indexListSQL())) {
        delete stmt;
        return nullptr;
    }
    stmt->bindString(0, tableName);

    if (!stmt->execute(*fbd->db)) {
        delete stmt;
        return nullptr;
    }

    auto *cd = NewCursorData(fbd->db, stmt);
    return REALNewRowSetFromDBCursor((dbCursor *)cd, &sFirebirdCursor);
}

static REALdbCursor fbEngineDirectSQLSelect(dbDatabase *dbData, REALstring sql) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return nullptr;

    auto *stmt = new FBStatement();
    if (!stmt->prepare(*fbd->db, RealToStd(sql))) {
        delete stmt;
        return nullptr;
    }
    if (!stmt->execute(*fbd->db)) {
        delete stmt;
        return nullptr;
    }

    auto *cd = NewCursorData(fbd->db, stmt);
    return REALNewRowSetFromDBCursor((dbCursor *)cd, &sFirebirdCursor);
}

static void fbEngineDirectSQLExecute(dbDatabase *dbData, REALstring sql) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return;
    bool ok = fbd->db->executeImmediate(RealToStd(sql));
    if (ok && fbd->autoCommit && fbd->db->hasActiveTransaction()) {
        fbd->db->commit();
    }
}

static REALdbCursor fbEngineSelectSQL(dbDatabase *dbData, REALstring sql, REALarray params) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return nullptr;

    auto *stmt = new FBStatement();
    if (!stmt->prepare(*fbd->db, RealToStd(sql))) {
        delete stmt;
        return nullptr;
    }
    BindParams(stmt, fbd->db, params);
    if (!stmt->execute(*fbd->db)) {
        delete stmt;
        return nullptr;
    }

    auto *cd = NewCursorData(fbd->db, stmt);
    return REALNewRowSetFromDBCursor((dbCursor *)cd, &sFirebirdCursor);
}

static void fbEngineExecuteSQL(dbDatabase *dbData, REALstring sql, REALarray params) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return;

    auto stmt = FBStatement();
    if (!stmt.prepare(*fbd->db, RealToStd(sql))) return;
    BindParams(&stmt, fbd->db, params);
    bool ok = stmt.execute(*fbd->db);

    if (ok && fbd->autoCommit && fbd->db->hasActiveTransaction()) {
        fbd->db->commit();
    }
}

static long fbEngineGetLastErrorCode(dbDatabase *dbData) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db) return 0;
    return fbd->db->lastErrorCode();
}

static REALstring fbEngineGetLastErrorString(dbDatabase *dbData) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db) return StdToReal("");
    return StdToReal(fbd->db->lastErrorString());
}

static void fbEngineCommit(dbDatabase *dbData) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (fbd && fbd->db) {
        fbd->db->commit();
        fbd->autoCommit = true;   // Transaction ended — restore auto-commit
    }
}

static void fbEngineRollback(dbDatabase *dbData) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (fbd && fbd->db) {
        fbd->db->rollback();
        fbd->autoCommit = true;   // Transaction ended — restore auto-commit
    }
}

static void fbEngineBeginTransaction(dbDatabase *dbData) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (fbd && fbd->db) {
        fbd->db->beginTransaction();
        fbd->autoCommit = false;  // Explicit transaction — no auto-commit
    }
}

static REALobject fbEnginePrepareStatement(dbDatabase *dbData, REALstring sql) {
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return nullptr;

    auto *stmt = new FBStatement();
    if (!stmt->prepare(*fbd->db, RealToStd(sql))) {
        delete stmt;
        return nullptr;
    }

    REALobject obj = REALnewInstanceOfClass(&sFirebirdPreparedStmtClass);
    if (!obj) {
        delete stmt;
        return nullptr;
    }

    auto *psData = (FirebirdPreparedStmtData *)REALGetClassData(obj, &sFirebirdPreparedStmtClass);
    psData->db = fbd->db;
    psData->stmt = stmt;

    return obj;
}

// ============================================================================
// Cursor Implementation
// ============================================================================

static void fbCursorClose(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (cd) {
        if (cd->stmt) {
            cd->stmt->close();
            delete cd->stmt;
            cd->stmt = nullptr;
        }
        delete cd;
    }
}

static int fbCursorColumnCount(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt) return 0;
    return cd->stmt->columnCount();
}

static REALstring fbCursorColumnName(dbCursor *cursor, int col) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt) return StdToReal("");
    const auto &info = cd->stmt->columnInfo(col);
    const std::string &name = info.alias.empty() ? info.name : info.alias;
    return StdToReal(name);
}

static int fbCursorRowCount(dbCursor *cursor) {
    return -1; // Firebird forward-only cursors don't report row count
}

static void fbCursorColumnValue(dbCursor *cursor, int col, void **value,
                                 unsigned char *type, int *length)
{
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt) {
        *type = (unsigned char)dbTypeNull;
        *value = nullptr;
        *length = 0;
        return;
    }

    FBValue val = cd->stmt->columnValue(col);

    if (val.isNull) {
        *type = (unsigned char)dbTypeNull;
        *value = nullptr;
        *length = 0;
        return;
    }

    const auto &info = cd->stmt->columnInfo(col);
    short dtype = info.baseType();

    switch (dtype) {
        case SQL_TEXT:
        case SQL_VARYING: {
            REALstring rs = StdToReal(val.strVal);
            *type = (unsigned char)dbTypeREALstring;
            *length = (int)val.strVal.size();
            // Store REALstring in lastValue, pass pointer TO it
            // Framework dereferences: *(REALstring*)*value
            cd->lastValue = (void *)rs;
            cd->lastType = *type;
            *value = &cd->lastValue;
            return;
        }
#ifdef SQL_INT128
        case SQL_INT128:
#endif
#ifdef SQL_DEC16
        case SQL_DEC16:
#endif
#ifdef SQL_DEC34
        case SQL_DEC34:
#endif
#ifdef SQL_TIME_TZ
        case SQL_TIME_TZ:
#endif
#ifdef SQL_TIME_TZ_EX
        case SQL_TIME_TZ_EX:
#endif
#ifdef SQL_TIMESTAMP_TZ
        case SQL_TIMESTAMP_TZ:
#endif
#ifdef SQL_TIMESTAMP_TZ_EX
        case SQL_TIMESTAMP_TZ_EX:
#endif
        {
            REALstring rs = StdToReal(val.strVal);
            *type = (unsigned char)dbTypeREALstring;
            *length = (int)val.strVal.size();
            cd->lastValue = (void *)rs;
            cd->lastType = *type;
            *value = &cd->lastValue;
            return;
        }
        case SQL_SHORT: {
            if (info.sqlscale < 0) {
                double *d = (double *)malloc(sizeof(double));
                *d = val.dblVal;
                *type = (unsigned char)dbTypeDouble;
                *value = d;
                *length = sizeof(double);
            } else {
                int32_t *v = (int32_t *)malloc(sizeof(int32_t));
                *v = (int32_t)val.intVal;
                *type = (unsigned char)dbTypeLong;
                *value = v;
                *length = sizeof(int32_t);
            }
            cd->lastValue = *value;
            cd->lastType = *type;
            return;
        }
        case SQL_LONG: {
            if (info.sqlscale < 0) {
                double *d = (double *)malloc(sizeof(double));
                *d = val.dblVal;
                *type = (unsigned char)dbTypeDouble;
                *value = d;
                *length = sizeof(double);
            } else {
                int32_t *v = (int32_t *)malloc(sizeof(int32_t));
                *v = (int32_t)val.intVal;
                *type = (unsigned char)dbTypeLong;
                *value = v;
                *length = sizeof(int32_t);
            }
            cd->lastValue = *value;
            cd->lastType = *type;
            return;
        }
        case SQL_INT64: {
            if (info.sqlscale < 0) {
                double *d = (double *)malloc(sizeof(double));
                *d = val.dblVal;
                *type = (unsigned char)dbTypeDouble;
                *value = d;
                *length = sizeof(double);
            } else {
                int64_t *v = (int64_t *)malloc(sizeof(int64_t));
                *v = val.intVal;
                *type = (unsigned char)dbTypeInt64;
                *value = v;
                *length = sizeof(int64_t);
            }
            cd->lastValue = *value;
            cd->lastType = *type;
            return;
        }
        case SQL_FLOAT:
        case SQL_DOUBLE: {
            double *d = (double *)malloc(sizeof(double));
            *d = val.dblVal;
            *type = (unsigned char)dbTypeDouble;
            *value = d;
            *length = sizeof(double);
            cd->lastValue = *value;
            cd->lastType = *type;
            return;
        }
        case SQL_TYPE_DATE: {
            struct tm t;
            memset(&t, 0, sizeof(t));
            isc_decode_sql_date(&val.dateVal, &t);
            dbDate *dd = (dbDate *)malloc(sizeof(dbDate));
            dd->year = ToXojoDbShort((short)(t.tm_year + 1900));
            dd->month = ToXojoDbShort((short)(t.tm_mon + 1));
            dd->day = ToXojoDbShort((short)t.tm_mday);
            *type = (unsigned char)dbTypeDate;
            *value = dd;
            *length = sizeof(dbDate);
            cd->lastValue = *value;
            cd->lastType = *type;
            return;
        }
        case SQL_TYPE_TIME: {
            struct tm t;
            memset(&t, 0, sizeof(t));
            isc_decode_sql_time(&val.timeVal, &t);
            dbTime *dt = (dbTime *)malloc(sizeof(dbTime));
            dt->hour = ToXojoDbShort((short)t.tm_hour);
            dt->minute = ToXojoDbShort((short)t.tm_min);
            dt->second = ToXojoDbShort((short)t.tm_sec);
            *type = (unsigned char)dbTypeTime;
            *value = dt;
            *length = sizeof(dbTime);
            cd->lastValue = *value;
            cd->lastType = *type;
            return;
        }
        case SQL_TIMESTAMP: {
            struct tm t;
            memset(&t, 0, sizeof(t));
            isc_decode_timestamp(&val.tsVal, &t);
            dbTimeStamp *ts = (dbTimeStamp *)malloc(sizeof(dbTimeStamp));
            ts->year = ToXojoDbShort((short)(t.tm_year + 1900));
            ts->month = ToXojoDbShort((short)(t.tm_mon + 1));
            ts->day = ToXojoDbShort((short)t.tm_mday);
            ts->hour = ToXojoDbShort((short)t.tm_hour);
            ts->minute = ToXojoDbShort((short)t.tm_min);
            ts->second = ToXojoDbShort((short)t.tm_sec);
            *type = (unsigned char)dbTypeTimeStamp;
            *value = ts;
            *length = sizeof(dbTimeStamp);
            cd->lastValue = *value;
            cd->lastType = *type;
            return;
        }
        case SQL_BLOB: {
            if (cd->db && info.sqlsubtype == 1) {
                // Text BLOB — read with UTF-8 transliteration
                std::string blobData;
                cd->db->readBlob(val.blobId, blobData, true);
                REALstring rs = StdToReal(blobData);
                *type = (unsigned char)dbTypeREALstring;
                *length = (int)blobData.size();
                cd->lastValue = (void *)rs;
                cd->lastType = *type;
                *value = &cd->lastValue;
            } else if (cd->db) {
                // Binary BLOB — return raw bytes
                std::string blobData;
                cd->db->readBlob(val.blobId, blobData, false);
                *length = (int)blobData.size();
                char *buf = (char *)malloc(*length);
                memcpy(buf, blobData.c_str(), *length);
                *type = (unsigned char)dbTypeBinary;
                *value = buf;
                cd->lastValue = *value;
                cd->lastType = *type;
            } else {
                *type = (unsigned char)dbTypeNull;
                *value = nullptr;
                *length = 0;
            }
            return;
        }
#ifdef SQL_BOOLEAN
        case SQL_BOOLEAN: {
            RBBoolean *b = (RBBoolean *)malloc(sizeof(RBBoolean));
            *b = val.intVal ? 1 : 0;
            *type = (unsigned char)dbTypeBoolean;
            *value = b;
            *length = sizeof(RBBoolean);
            cd->lastValue = *value;
            cd->lastType = *type;
            return;
        }
#endif
        default: {
            REALstring rs = StdToReal(val.strVal);
            *type = (unsigned char)dbTypeREALstring;
            *length = (int)val.strVal.size();
            cd->lastValue = (void *)rs;
            cd->lastType = *type;
            *value = &cd->lastValue;
            return;
        }
    }
}

static void fbCursorReleaseValue(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->lastValue) return;

    if (cd->lastType == (unsigned char)dbTypeREALstring) {
        REALUnlockString((REALstring)cd->lastValue);
    } else {
        free(cd->lastValue);
    }
    cd->lastValue = nullptr;
    cd->lastType = (unsigned char)dbTypeNull;
}

static RBBoolean fbCursorNextRow(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt) return false;

    cd->bof = false;
    if (cd->stmt->fetch()) {
        cd->eof = false;
        cd->firstRowCalled = true;
        return true;
    }
    cd->eof = true;
    return false;
}

static int fbCursorColumnType(dbCursor *cursor, int index) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt || index < 0 || index >= cd->stmt->columnCount())
        return (int)dbTypeNull;

    const auto &info = cd->stmt->columnInfo(index);
    return (int)FirebirdTypeToXojo(info.sqltype, info.sqlscale, info.sqlsubtype);
}

static RBBoolean fbCursorIsBOF(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    return cd ? cd->bof : true;
}

static RBBoolean fbCursorIsEOF(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    return cd ? cd->eof : true;
}

// ============================================================================
// FirebirdDatabase Class Implementation
// ============================================================================

static void fbClassConstructor(REALobject instance) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);
    data->host = nullptr;
    data->databaseName = nullptr;
    data->userName = nullptr;
    data->password = nullptr;
    data->characterSet = nullptr;
    data->role = nullptr;
    data->port = 3050;
    data->dialect = 3;
}

static void fbClassDestructor(REALobject instance) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);
    if (data->host) REALUnlockString(data->host);
    if (data->databaseName) REALUnlockString(data->databaseName);
    if (data->userName) REALUnlockString(data->userName);
    if (data->password) REALUnlockString(data->password);
    if (data->characterSet) REALUnlockString(data->characterSet);
    if (data->role) REALUnlockString(data->role);
}

static RBBoolean fbClassConnect(REALobject instance) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);

    std::string host = RealToStd(data->host);
    std::string dbName = RealToStd(data->databaseName);
    std::string user = RealToStd(data->userName);
    std::string pass = RealToStd(data->password);
    std::string charset = RealToStd(data->characterSet);
    std::string role = RealToStd(data->role);

    if (charset.empty()) charset = "UTF8";
    if (user.empty()) user = "SYSDBA";

    // Build connection string: host/port:database or just database path
    std::string connStr;
    if (!host.empty()) {
        connStr = host;
        if (data->port != 3050 && data->port > 0) {
            connStr += "/" + std::to_string(data->port);
        }
        connStr += ":" + dbName;
    } else {
        connStr = dbName;
    }

    auto *fb = new FBDatabase();
    if (!fb->connect(connStr, user, pass, charset, role, data->dialect)) {
        delete fb;
        return false;
    }

    auto *fbd = new FirebirdDbData;
    fbd->db = fb;
    fbd->autoCommit = true;

    // Wire our native database into this Database instance.
    // Since FirebirdDatabase inherits from Database, the REALobject
    // IS a REALdbDatabase — the cast is safe for Database subclasses.
    REALdbDatabase realDb = (REALdbDatabase)instance;
    REALConstructDBDatabase(realDb, (dbDatabase *)fbd, &sFirebirdEngine);
    REALSetDBIsConnected(realDb, true);

    return true;
}

static REALstring fbClassServerVersion(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return StdToReal("");

    std::string value;
    if (!fbd->db->serverVersion(value)) return StdToReal("");
    return StdToReal(value);
}

static RBInteger fbClassPageSize(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return 0;

    long value = 0;
    if (!fbd->db->pageSize(value)) return 0;
    return (RBInteger)value;
}

static RBInteger fbClassDatabaseSQLDialect(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return 0;

    long value = 0;
    if (!fbd->db->databaseSQLDialect(value)) return 0;
    return (RBInteger)value;
}

static REALstring fbClassODSVersion(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return StdToReal("");

    std::string value;
    if (!fbd->db->odsVersion(value)) return StdToReal("");
    return StdToReal(value);
}

static RBBoolean fbClassIsReadOnly(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;

    bool value = false;
    if (!fbd->db->isReadOnly(value)) return false;
    return value;
}

static RBBoolean fbClassHasActiveTransaction(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->hasActiveTransaction();
}

static RBInt64 fbClassTransactionID(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db || !fbd->db->hasActiveTransaction()) return 0;

    int64_t value = 0;
    if (!fbd->db->transactionID(value)) return 0;
    return (RBInt64)value;
}

static REALstring fbClassTransactionIsolation(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db || !fbd->db->hasActiveTransaction()) return StdToReal("");

    std::string value;
    if (!fbd->db->transactionIsolation(value)) return StdToReal("");
    return StdToReal(value);
}

static REALstring fbClassTransactionAccessMode(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db || !fbd->db->hasActiveTransaction()) return StdToReal("");

    std::string value;
    if (!fbd->db->transactionAccessMode(value)) return StdToReal("");
    return StdToReal(value);
}

static RBInteger fbClassTransactionLockTimeout(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db || !fbd->db->hasActiveTransaction()) return -2;

    long value = -1;
    if (!fbd->db->transactionLockTimeout(value)) return -2;
    return (RBInteger)value;
}

static RBBoolean fbClassBeginTransactionWithOptions(REALobject instance, REALstring isolation, RBBoolean readOnly, RBInteger lockTimeout) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;

    const std::string isolationText = isolation ? RealToStd(isolation) : "";
    if (!fbd->db->beginTransactionWithOptions(isolationText, readOnly != 0, (long)lockTimeout)) {
        return false;
    }

    fbd->autoCommit = false;
    return true;
}

// ============================================================================
// FirebirdPreparedStatement Class Implementation
// ============================================================================

static void fbPrepStmtConstructor(REALobject instance) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    data->db = nullptr;
    data->stmt = nullptr;
}

static void fbPrepStmtDestructor(REALobject instance) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (data->stmt) {
        data->stmt->close();
        delete data->stmt;
        data->stmt = nullptr;
    }
}

static void fbPrepStmtBindString(REALobject instance, int32_t index, REALstring value) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->stmt) return;
    if (value) {
        data->stmt->bindString(index, RealToStd(value));
    } else {
        data->stmt->bindNull(index);
    }
}

static void fbPrepStmtBindInt(REALobject instance, int32_t index, RBInt64 value) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->stmt) return;
    data->stmt->bindInt(index, value);
}

static void fbPrepStmtBindDouble(REALobject instance, int32_t index, double value) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->stmt) return;
    data->stmt->bindDouble(index, value);
}

static void fbPrepStmtBindBoolean(REALobject instance, int32_t index, RBBoolean value) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->stmt) return;
    data->stmt->bindBoolean(index, value != 0);
}

static void fbPrepStmtBindDateTime(REALobject instance, int32_t index, REALobject value) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->stmt) return;

    if (!value) {
        data->stmt->bindNull(index);
        return;
    }

    if (!BindDateTimeValue(data->stmt, index, value)) {
        data->stmt->bindNull(index);
    }
}

static void fbPrepStmtBindTextBlob(REALobject instance, int32_t index, REALstring value) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->stmt) return;

    if (!value) {
        data->stmt->bindNull(index);
        return;
    }

    if (!BindTextBlobValue(data->stmt, data->db, index, value)) {
        data->stmt->bindNull(index);
    }
}

static void fbPrepStmtBindBinaryBlob(REALobject instance, int32_t index, REALobject value) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->stmt) return;

    if (!value) {
        data->stmt->bindNull(index);
        return;
    }

    if (!BindBinaryBlobValue(data->stmt, data->db, index, value)) {
        data->stmt->bindNull(index);
    }
}

static void fbPrepStmtBindNull(REALobject instance, int32_t index) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->stmt) return;
    data->stmt->bindNull(index);
}

static REALdbCursor fbPrepStmtSelectSQL(REALobject instance) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->db || !data->stmt) return nullptr;

    if (!data->stmt->execute(*data->db)) return nullptr;

    // Transfer ownership of the statement to the cursor
    auto *cd = NewCursorData(data->db, data->stmt);
    data->stmt = nullptr;

    return REALNewRowSetFromDBCursor((dbCursor *)cd, &sFirebirdCursor);
}

static void fbPrepStmtExecuteSQL(REALobject instance) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->db || !data->stmt) return;
    data->stmt->execute(*data->db);
}

// ============================================================================
// PluginEntry — called by the Xojo runtime via PluginMain.cpp glue
// ============================================================================

void PluginEntry() {
    REALRegisterDBEngine(&sFirebirdEngine);
    REALRegisterDBCursor(&sFirebirdCursor);

    SetClassConsoleSafe(&sFirebirdDatabaseClass);
    REALRegisterClass(&sFirebirdDatabaseClass);

    SetClassConsoleSafe(&sFirebirdPreparedStmtClass);
    REALRegisterClass(&sFirebirdPreparedStmtClass);
}
