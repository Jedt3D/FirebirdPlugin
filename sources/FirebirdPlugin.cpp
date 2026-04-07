// FirebirdPlugin.cpp — Xojo Plugin bridge for Firebird SQL
// Implements REALdbEngineDefinition, REALdbCursorDefinition,
// FirebirdDatabase class, and PluginEntry().

#include "FirebirdPlugin.h"

#if defined(_WIN64) && defined(__aarch64__)
// ARM64 builds use dynamic loading for cross-architecture support
#include "FirebirdLoader.h"

// Macros to redirect function calls to pointers for ARM64 builds
#define isc_cancel_events ptr_isc_cancel_events
#define isc_que_events ptr_isc_que_events
#define isc_event_block ptr_isc_event_block
#define isc_event_counts ptr_isc_event_counts
#define isc_free ptr_isc_free
#define isc_encode_sql_date ptr_isc_encode_sql_date
#define isc_encode_sql_time ptr_isc_encode_sql_time
#define isc_encode_timestamp ptr_isc_encode_timestamp
#define isc_decode_sql_date ptr_isc_decode_sql_date
#define isc_decode_sql_time ptr_isc_decode_sql_time
#define isc_decode_timestamp ptr_isc_decode_timestamp
#define isc_sqlcode ptr_isc_sqlcode
#define fb_interpret ptr_fb_interpret
#endif

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>
#include <ctime>
#include <vector>

// ============================================================================
// Forward declarations — all callback functions
// ============================================================================

// DB Engine callbacks
static void         fbEngineClose(dbDatabase *);
static REALdbCursor fbEngineTableSchema(dbDatabase *);
static REALdbCursor fbEngineFieldSchema(dbDatabase *, REALstring table);
static REALdbCursor fbEngineDirectSQLSelect(dbDatabase *, REALstring sql);
static void         fbEngineDirectSQLExecute(dbDatabase *, REALstring sql);
static void         fbEngineAddTableRecord(dbDatabase *, REALstring tableName, REALcolumnValue *values);
static REALdbCursor fbEngineSelectSQL(dbDatabase *, REALstring sql, REALarray params);
static void         fbEngineExecuteSQL(dbDatabase *, REALstring sql, REALarray params);
static RBInteger    fbEngineAddRowWithReturnValue(dbDatabase *, REALstring tableName, REALcolumnValue *values, REALstring idColumnName);
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
static void         fbCursorPrevRow(dbCursor *);
static void         fbCursorFirstRow(dbCursor *);
static void         fbCursorLastRow(dbCursor *);
static int          fbCursorColumnType(dbCursor *, int index);
static RBBoolean    fbCursorIsBOF(dbCursor *);
static RBBoolean    fbCursorIsEOF(dbCursor *);

// FirebirdDatabase class methods
static void         fbClassConstructor(REALobject instance);
static void         fbClassDestructor(REALobject instance);
static RBBoolean    fbClassConnect(REALobject instance);
static void         fbClassListen(REALobject instance, REALstring name);
static void         fbClassStopListening(REALobject instance, REALstring name);
static void         fbClassCheckForNotifications(REALobject instance);
static void         fbClassNotify(REALobject instance, REALstring name);
static REALobject   fbClassCreateBlob(REALobject instance);
static REALobject   fbClassOpenBlob(REALobject instance, REALdbCursor rowset, REALstring column);
static REALstring   fbClassWireCryptGet(REALobject instance, long param);
static void         fbClassWireCryptSet(REALobject instance, long param, REALstring value);
static RBInteger    fbClassSSLModeGet(REALobject instance, long param);
static void         fbClassSSLModeSet(REALobject instance, long param, RBInteger value);
static REALstring   fbClassServerVersion(REALobject instance);
static RBInteger    fbClassPageSize(REALobject instance);
static RBInteger    fbClassDatabaseSQLDialect(REALobject instance);
static REALstring   fbClassODSVersion(REALobject instance);
static RBBoolean    fbClassIsReadOnly(REALobject instance);
static RBInt64      fbClassAffectedRowCount(REALobject instance);
static RBBoolean    fbClassHasActiveTransaction(REALobject instance);
static RBInt64      fbClassTransactionID(REALobject instance);
static REALstring   fbClassTransactionIsolation(REALobject instance);
static REALstring   fbClassTransactionAccessMode(REALobject instance);
static RBInteger    fbClassTransactionLockTimeout(REALobject instance);
static RBBoolean    fbClassBeginTransactionWithOptions(REALobject instance, REALstring isolation, RBBoolean readOnly, RBInteger lockTimeout);
static RBBoolean    fbClassBackupDatabase(REALobject instance, REALstring backupFile);
static RBBoolean    fbClassRestoreDatabase(REALobject instance, REALstring backupFile, REALstring targetDatabase, RBBoolean replaceExisting);
static RBBoolean    fbClassDatabaseStatistics(REALobject instance);
static RBBoolean    fbClassValidateDatabase(REALobject instance);
static RBBoolean    fbClassSweepDatabase(REALobject instance);
static RBBoolean    fbClassListLimboTransactions(REALobject instance);
static RBBoolean    fbClassCommitLimboTransaction(REALobject instance, RBInt64 transactionId);
static RBBoolean    fbClassRollbackLimboTransaction(REALobject instance, RBInt64 transactionId);
static RBBoolean    fbClassSetSweepInterval(REALobject instance, long interval);
static RBBoolean    fbClassShutdownDenyNewAttachments(REALobject instance, long timeoutSeconds);
static RBBoolean    fbClassBringDatabaseOnline(REALobject instance);
static RBBoolean    fbClassDisplayUsers(REALobject instance);
static RBBoolean    fbClassAddUser(REALobject instance, REALstring userName, REALstring password);
static RBBoolean    fbClassChangeUserPassword(REALobject instance, REALstring userName, REALstring password);
static RBBoolean    fbClassSetUserAdmin(REALobject instance, REALstring userName, RBBoolean isAdmin);
static RBBoolean    fbClassUpdateUserNames(REALobject instance, REALstring userName, REALstring firstName, REALstring middleName, REALstring lastName);
static RBBoolean    fbClassDeleteUser(REALobject instance, REALstring userName);
static REALstring   fbClassLastServiceOutput(REALobject instance);

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
static void         fbPrepStmtBindBlob(REALobject instance, int32_t index, REALobject value);
static void         fbPrepStmtBindNull(REALobject instance, int32_t index);
static REALdbCursor fbPrepStmtSelectSQL(REALobject instance);
static void         fbPrepStmtExecuteSQL(REALobject instance);

// FirebirdBlob class methods
static void         fbBlobConstructor(REALobject instance);
static void         fbBlobDestructor(REALobject instance);
static REALstring   fbBlobIdGet(REALobject instance, long param);
static RBInt64      fbBlobLengthGet(REALobject instance, long param);
static RBInt64      fbBlobPositionGet(REALobject instance, long param);
static RBBoolean    fbBlobIsOpenGet(REALobject instance, long param);
static REALobject   fbBlobRead(REALobject instance, int32_t count);
static RBBoolean    fbBlobWrite(REALobject instance, REALobject value);
static RBInt64      fbBlobSeek(REALobject instance, RBInt64 offset, int32_t whence);
static RBBoolean    fbBlobClose(REALobject instance);

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
    fbEngineAddTableRecord,
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
    fbEngineAddRowWithReturnValue,
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
    fbCursorPrevRow,
    fbCursorFirstRow,
    fbCursorLastRow,
    fbCursorColumnType,
    fbCursorIsBOF,
    fbCursorIsEOF,
    nullptr,                                 // dummy10
};

// --- FirebirdDatabase Class -------------------------------------------------

static REALevent sFirebirdClassEvents[] = {
    { "ReceivedNotification(name As String, count As Integer)" },
};

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
    { "", "WireCrypt", "String", REALconsoleSafe, (REALproc)fbClassWireCryptGet, (REALproc)fbClassWireCryptSet, 0 },
    { "", "AuthClientPlugins", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, authClientPlugins) },
    { "", "SSLMode", "Integer", REALconsoleSafe, (REALproc)fbClassSSLModeGet, (REALproc)fbClassSSLModeSet, 0 },
    { "", "Dialect", "Integer", REALconsoleSafe, REALstandardGetter, REALstandardSetter,
      FieldOffset(FirebirdClassData, dialect) },
    { "", "AffectedRowCount", "Int64", REALconsoleSafe, (REALproc)fbClassAffectedRowCount, nullptr, 0 },
};

static REALmethodDefinition sFirebirdClassMethods[] = {
    { (REALproc)fbClassConnect, REALnoImplementation, "Connect() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassListen, REALnoImplementation, "Listen(name As String)", REALconsoleSafe },
    { (REALproc)fbClassStopListening, REALnoImplementation, "StopListening(name As String)", REALconsoleSafe },
    { (REALproc)fbClassCheckForNotifications, REALnoImplementation, "CheckForNotifications()", REALconsoleSafe },
    { (REALproc)fbClassNotify, REALnoImplementation, "Notify(name As String)", REALconsoleSafe },
    { (REALproc)fbClassCreateBlob, REALnoImplementation, "CreateBlob() As FirebirdBlob", REALconsoleSafe },
    { (REALproc)fbClassOpenBlob, REALnoImplementation, "OpenBlob(rowset As RowSet, column As String) As FirebirdBlob", REALconsoleSafe },
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
    { (REALproc)fbClassBackupDatabase, REALnoImplementation, "BackupDatabase(backupFile As String) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassRestoreDatabase, REALnoImplementation, "RestoreDatabase(backupFile As String, targetDatabase As String, replaceExisting As Boolean) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassDatabaseStatistics, REALnoImplementation, "DatabaseStatistics() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassValidateDatabase, REALnoImplementation, "ValidateDatabase() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassSweepDatabase, REALnoImplementation, "SweepDatabase() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassListLimboTransactions, REALnoImplementation, "ListLimboTransactions() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassCommitLimboTransaction, REALnoImplementation, "CommitLimboTransaction(transactionId As Int64) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassRollbackLimboTransaction, REALnoImplementation, "RollbackLimboTransaction(transactionId As Int64) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassSetSweepInterval, REALnoImplementation, "SetSweepInterval(interval As Integer) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassShutdownDenyNewAttachments, REALnoImplementation, "ShutdownDenyNewAttachments(timeoutSeconds As Integer) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassBringDatabaseOnline, REALnoImplementation, "BringDatabaseOnline() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassDisplayUsers, REALnoImplementation, "DisplayUsers() As Boolean", REALconsoleSafe },
    { (REALproc)fbClassAddUser, REALnoImplementation, "AddUser(userName As String, password As String) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassChangeUserPassword, REALnoImplementation, "ChangeUserPassword(userName As String, password As String) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassSetUserAdmin, REALnoImplementation, "SetUserAdmin(userName As String, isAdmin As Boolean) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassUpdateUserNames, REALnoImplementation, "UpdateUserNames(userName As String, firstName As String, middleName As String, lastName As String) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassDeleteUser, REALnoImplementation, "DeleteUser(userName As String) As Boolean", REALconsoleSafe },
    { (REALproc)fbClassLastServiceOutput, REALnoImplementation, "LastServiceOutput() As String", REALconsoleSafe },
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
    sFirebirdClassEvents,
    sizeof(sFirebirdClassEvents) / sizeof(REALevent),
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
    { (REALproc)fbPrepStmtBindBlob, REALnoImplementation,
      "BindBlob(index As Integer, value As FirebirdBlob)", REALconsoleSafe },
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

// --- FirebirdBlob Class -----------------------------------------------------

static REALproperty sFirebirdBlobProperties[] = {
    { "", "BlobId", "String", REALconsoleSafe, (REALproc)fbBlobIdGet, nullptr, 0 },
    { "", "Length", "Int64", REALconsoleSafe, (REALproc)fbBlobLengthGet, nullptr, 0 },
    { "", "Position", "Int64", REALconsoleSafe, (REALproc)fbBlobPositionGet, nullptr, 0 },
    { "", "IsOpen", "Boolean", REALconsoleSafe, (REALproc)fbBlobIsOpenGet, nullptr, 0 },
};

static REALmethodDefinition sFirebirdBlobMethods[] = {
    { (REALproc)fbBlobRead, REALnoImplementation,
      "Read(count As Integer) As MemoryBlock", REALconsoleSafe },
    { (REALproc)fbBlobWrite, REALnoImplementation,
      "Write(value As MemoryBlock) As Boolean", REALconsoleSafe },
    { (REALproc)fbBlobSeek, REALnoImplementation,
      "Seek(offset As Int64, whence As Integer) As Int64", REALconsoleSafe },
    { (REALproc)fbBlobClose, REALnoImplementation,
      "Close() As Boolean", REALconsoleSafe },
};

static REALclassDefinition sFirebirdBlobClass = {
    kCurrentREALControlVersion,
    "FirebirdBlob",                          // name
    nullptr,                                  // superName
    sizeof(FirebirdBlobData),                // dataSize
    0,                                       // forSystemUse
    (REALproc)fbBlobConstructor,
    (REALproc)fbBlobDestructor,
    sFirebirdBlobProperties,
    sizeof(sFirebirdBlobProperties) / sizeof(REALproperty),
    sFirebirdBlobMethods,
    sizeof(sFirebirdBlobMethods) / sizeof(REALmethodDefinition),
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

static constexpr uint32_t kFirebirdCursorSignature = 0x46424355;

static std::string BlobIdToString(const ISC_QUAD &blobId) {
    const uint32_t high = (uint32_t)blobId.gds_quad_high;
    const uint32_t low = (uint32_t)blobId.gds_quad_low;
    return std::to_string(high) + ":" + std::to_string(low);
}

static FirebirdCursorData *GetFirebirdCursorData(REALdbCursor rowset) {
    dbCursor *cursor = REALGetCursorFromREALdbCursor(rowset);
    if (!cursor) return nullptr;

    auto *cd = reinterpret_cast<FirebirdCursorData *>(cursor);
    if (cd->signature != kFirebirdCursorSignature) return nullptr;
    return cd;
}

static int FindCursorColumnIndex(FirebirdCursorData *cd, const std::string &columnName) {
    if (!cd || !cd->stmt) return -1;

    std::string wanted = columnName;
    while (!wanted.empty() && std::isspace((unsigned char)wanted.front())) wanted.erase(wanted.begin());
    while (!wanted.empty() && std::isspace((unsigned char)wanted.back())) wanted.pop_back();
    for (auto &ch : wanted) ch = (char)std::toupper((unsigned char)ch);

    const int count = cd->stmt->columnCount();
    for (int i = 0; i < count; i++) {
        const auto &info = cd->stmt->columnInfo(i);
        std::string alias = info.alias.empty() ? info.name : info.alias;
        std::string name = info.name;
        for (auto &ch : alias) ch = (char)std::toupper((unsigned char)ch);
        for (auto &ch : name) ch = (char)std::toupper((unsigned char)ch);
        if (alias == wanted || name == wanted) return i;
    }

    return -1;
}

static REALobject NewFirebirdBlobObject(FBBlob *blob) {
    if (!blob) return nullptr;

    REALobject obj = REALnewInstanceOfClass(&sFirebirdBlobClass);
    if (!obj) {
        delete blob;
        return nullptr;
    }

    auto *blobData = (FirebirdBlobData *)REALGetClassData(obj, &sFirebirdBlobClass);
    blobData->blob = blob;
    return obj;
}

static void SetRealStringField(REALstring &field, const std::string &value) {
    if (field) REALUnlockString(field);
    field = StdToReal(value);
}

static std::string NormalizeLowerToken(const std::string &text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (char ch : text) {
        normalized.push_back((char)std::tolower((unsigned char)ch));
    }
    return normalized;
}

static int32_t WireCryptToSSLModeValue(const std::string &wireCrypt) {
    const std::string normalized = NormalizeLowerToken(wireCrypt);
    if (normalized.empty()) return -1;
    if (normalized == "disabled") return 0;
    if (normalized == "enabled") return 2;
    if (normalized == "required") return 3;
    return -1;
}

static bool SSLModeToWireCryptValue(RBInteger sslMode, std::string &wireCrypt) {
    switch ((int32_t)sslMode) {
        case -1:
            wireCrypt.clear();
            return true;
        case 0:
            wireCrypt = "Disabled";
            return true;
        case 1:
        case 2:
            wireCrypt = "Enabled";
            return true;
        case 3:
            wireCrypt = "Required";
            return true;
        default:
            return false;
    }
}

static bool ResolveEffectiveWireCrypt(const FirebirdClassData *data, std::string &wireCrypt) {
    if (!data) {
        wireCrypt.clear();
        return true;
    }

    wireCrypt = RealToStd(data->wireCrypt);
    if (!wireCrypt.empty()) return true;
    if (data->sslMode < 0) return true;
    return SSLModeToWireCryptValue(data->sslMode, wireCrypt);
}

static FirebirdDbData *GetFirebirdDbData(REALobject instance) {
    if (!instance) return nullptr;
    REALdbDatabase realDb = (REALdbDatabase)instance;
    return (FirebirdDbData *)REALGetDBFromREALdbDatabase(realDb);
}

static FirebirdDbData *EnsureFirebirdDbData(REALobject instance) {
    if (!instance) return nullptr;

    FirebirdDbData *fbd = GetFirebirdDbData(instance);
    if (fbd) return fbd;

    auto *fb = new FBDatabase();
    auto *newData = new FirebirdDbData;
    newData->db = fb;
    newData->autoCommit = true;
    newData->events = nullptr;

    REALdbDatabase realDb = (REALdbDatabase)instance;
    REALConstructDBDatabase(realDb, (dbDatabase *)newData, &sFirebirdEngine);
    REALSetDBIsConnected(realDb, false);
    return newData;
}

struct FirebirdPendingNotification {
    std::string name;
    ISC_ULONG count = 0;
};

struct FirebirdEventState {
    FirebirdDbData *owner = nullptr;
    isc_db_handle *dbHandle = nullptr;
    ISC_SCHAR *eventBuffer = nullptr;
    ISC_SCHAR *resultBuffer = nullptr;
    ISC_USHORT bufferLength = 0;
    ISC_LONG eventId = 0;
    bool armed = false;
    std::vector<std::string> names;
    std::vector<FirebirdPendingNotification> pending;
    std::mutex mutex;
    std::atomic<bool> closing{false};
    std::atomic<int> callbackDepth{0};
    std::atomic<uint64_t> generation{0};
};

static FirebirdEventState *EnsureEventState(FirebirdDbData *fbd) {
    if (!fbd) return nullptr;
    if (!fbd->events) {
        auto *state = new FirebirdEventState;
        state->owner = fbd;
        if (fbd->db) state->dbHandle = &fbd->db->dbHandle();
        fbd->events = state;
    }
    return fbd->events;
}

static std::string TrimASCII(const std::string &text) {
    size_t start = 0;
    while (start < text.size() && std::isspace((unsigned char)text[start])) start = start + 1;

    size_t finish = text.size();
    while (finish > start && std::isspace((unsigned char)text[finish - 1])) finish = finish - 1;

    return text.substr(start, finish - start);
}

static std::string EscapeSqlLiteral(const std::string &text) {
    std::string escaped;
    escaped.reserve(text.size() + 8);
    for (char ch : text) {
        escaped.push_back(ch);
        if (ch == '\'') escaped.push_back('\'');
    }
    return escaped;
}

static void SetStatusError(FBDatabase *db, ISC_STATUS *status) {
    if (!db) return;

    char buffer[512];
    std::ostringstream message;
    const ISC_STATUS *vector = status;
    const long code = isc_sqlcode(status);

    while (fb_interpret(buffer, sizeof(buffer), &vector)) {
        if (!message.str().empty()) message << '\n';
        message << buffer;
    }

    db->setError(code, message.str().empty() ? "Firebird event API call failed" : message.str());
}

static void FreeEventBuffers(FirebirdEventState *state) {
    if (!state) return;

    if (state->eventBuffer) {
        isc_free(state->eventBuffer);
        state->eventBuffer = nullptr;
    }

    if (state->resultBuffer) {
        isc_free(state->resultBuffer);
        state->resultBuffer = nullptr;
    }

    state->bufferLength = 0;
}

static bool BuildEventBlock(FBDatabase *db, FirebirdEventState *state) {
    if (!db || !state) return false;

    switch (state->names.size()) {
        case 0:
            state->bufferLength = 0;
            return true;
        case 1:
            state->bufferLength = (ISC_USHORT)isc_event_block((ISC_UCHAR **)&state->eventBuffer, (ISC_UCHAR **)&state->resultBuffer,
                                                              1, state->names[0].c_str());
            return true;
        case 2:
            state->bufferLength = (ISC_USHORT)isc_event_block((ISC_UCHAR **)&state->eventBuffer, (ISC_UCHAR **)&state->resultBuffer,
                                                              2, state->names[0].c_str(), state->names[1].c_str());
            return true;
        case 3:
            state->bufferLength = (ISC_USHORT)isc_event_block((ISC_UCHAR **)&state->eventBuffer, (ISC_UCHAR **)&state->resultBuffer,
                                                              3, state->names[0].c_str(), state->names[1].c_str(), state->names[2].c_str());
            return true;
        case 4:
            state->bufferLength = (ISC_USHORT)isc_event_block((ISC_UCHAR **)&state->eventBuffer, (ISC_UCHAR **)&state->resultBuffer,
                                                              4, state->names[0].c_str(), state->names[1].c_str(), state->names[2].c_str(), state->names[3].c_str());
            return true;
        case 5:
            state->bufferLength = (ISC_USHORT)isc_event_block((ISC_UCHAR **)&state->eventBuffer, (ISC_UCHAR **)&state->resultBuffer,
                                                              5, state->names[0].c_str(), state->names[1].c_str(), state->names[2].c_str(), state->names[3].c_str(), state->names[4].c_str());
            return true;
        case 6:
            state->bufferLength = (ISC_USHORT)isc_event_block((ISC_UCHAR **)&state->eventBuffer, (ISC_UCHAR **)&state->resultBuffer,
                                                              6, state->names[0].c_str(), state->names[1].c_str(), state->names[2].c_str(), state->names[3].c_str(), state->names[4].c_str(), state->names[5].c_str());
            return true;
        case 7:
            state->bufferLength = (ISC_USHORT)isc_event_block((ISC_UCHAR **)&state->eventBuffer, (ISC_UCHAR **)&state->resultBuffer,
                                                              7, state->names[0].c_str(), state->names[1].c_str(), state->names[2].c_str(), state->names[3].c_str(), state->names[4].c_str(), state->names[5].c_str(), state->names[6].c_str());
            return true;
        case 8:
            state->bufferLength = (ISC_USHORT)isc_event_block((ISC_UCHAR **)&state->eventBuffer, (ISC_UCHAR **)&state->resultBuffer,
                                                              8, state->names[0].c_str(), state->names[1].c_str(), state->names[2].c_str(), state->names[3].c_str(), state->names[4].c_str(), state->names[5].c_str(), state->names[6].c_str(), state->names[7].c_str());
            return true;
        default:
            db->setError(-200166, "A maximum of 8 Firebird event names can be listened for at once");
            return false;
    }
}

static void WaitForEventCallbacksToDrain(FirebirdEventState *state) {
    if (!state) return;

    while (state->callbackDepth.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

static void FireReceivedNotificationEvent(REALobject instance, const std::string &name, RBInteger count) {
    typedef void (*FirebirdNotificationEventTy)(REALobject instance, REALstring name, RBInteger count);

    FirebirdNotificationEventTy callback =
        (FirebirdNotificationEventTy)REALGetEventInstance((REALcontrolInstance)instance, &sFirebirdClassEvents[0]);
    if (!callback) return;

    REALstring eventName = StdToReal(name);
    callback(instance, eventName, count);
    if (eventName) REALUnlockString(eventName);
}

#if defined(_WIN64) && defined(__aarch64__)
// ARM64 uses cdecl calling convention for compatibility
static void FirebirdEventCallback(void *arg, ISC_USHORT length, const ISC_UCHAR *updated) {
#else
// Other platforms use ISC_EXPORT (which may be __stdcall or cdecl)
static void ISC_EXPORT FirebirdEventCallback(void *arg, ISC_USHORT length, const ISC_UCHAR *updated) {
#endif
    auto *state = static_cast<FirebirdEventState *>(arg);
    if (!state) return;

    state->callbackDepth.fetch_add(1);
    const uint64_t callbackGeneration = state->generation.load();

    if (state->closing.load()) {
        state->callbackDepth.fetch_sub(1);
        return;
    }

    isc_db_handle *dbHandle = nullptr;
    ISC_UCHAR *eventBuffer = nullptr;
    ISC_USHORT bufferLength = 0;

    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->armed = false;
        state->eventId = 0;

        if (state->resultBuffer && updated && !state->names.empty()) {
            std::vector<ISC_ULONG> counts(state->names.size(), 0);
            isc_event_counts(counts.data(), (short)length,
                             reinterpret_cast<ISC_UCHAR *>(state->resultBuffer), updated);

            for (size_t i = 0; i < counts.size(); ++i) {
                if (counts[i] == 0) continue;

                bool merged = false;
                for (auto &pending : state->pending) {
                    if (pending.name == state->names[i]) {
                        pending.count += counts[i];
                        merged = true;
                        break;
                    }
                }

                if (!merged) {
                    FirebirdPendingNotification pending;
                    pending.name = state->names[i];
                    pending.count = counts[i];
                    state->pending.push_back(pending);
                }
            }

            std::memcpy(state->resultBuffer, updated, length);
        }

        if (!state->closing.load()) {
            dbHandle = state->dbHandle;
            eventBuffer = reinterpret_cast<ISC_UCHAR *>(state->eventBuffer);
            bufferLength = state->bufferLength;
        }
    }

    if (!state->closing.load() && callbackGeneration == state->generation.load() &&
        dbHandle && *dbHandle != 0 && eventBuffer && bufferLength > 0) {
        ISC_STATUS_ARRAY status = {};
        ISC_LONG eventId = 0;
        if (!isc_que_events(status, dbHandle, &eventId, bufferLength,
                            eventBuffer,
                            FirebirdEventCallback, state)) {
            if (!state->closing.load() && callbackGeneration == state->generation.load()) {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->eventId = eventId;
                state->armed = true;
            } else if (dbHandle && *dbHandle != 0) {
                ISC_STATUS_ARRAY cancelStatus = {};
                isc_cancel_events(cancelStatus, dbHandle, &eventId);
            }
        }
    }

    state->callbackDepth.fetch_sub(1);
}

static void CancelEventListening(FirebirdDbData *fbd) {
    if (!fbd || !fbd->events) return;

    auto *state = fbd->events;
    state->closing.store(true);
    state->generation.fetch_add(1);

    ISC_LONG eventId = 0;
    bool armed = false;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        eventId = state->eventId;
        armed = state->armed;
        state->armed = false;
        state->eventId = 0;
    }

    if (armed && state->dbHandle && *state->dbHandle != 0) {
        ISC_STATUS_ARRAY status = {};
        isc_cancel_events(status, state->dbHandle, &eventId);
    }

    WaitForEventCallbacksToDrain(state);
}

static bool ArmEventListening(FirebirdDbData *fbd) {
    if (!fbd || !fbd->db) return false;

    auto *state = EnsureEventState(fbd);
    if (!state) return false;

    state->owner = fbd;
    state->dbHandle = &fbd->db->dbHandle();

    if (!fbd->db->isConnected()) {
        fbd->db->setError(-200160, "Database is not connected");
        return false;
    }

    CancelEventListening(fbd);
    FreeEventBuffers(state);
    state->pending.clear();
    state->eventId = 0;
    state->closing.store(false);

    if (state->names.empty()) {
        fbd->db->setError(0, "");
        return true;
    }

    if (!BuildEventBlock(fbd->db, state)) {
        FreeEventBuffers(state);
        return false;
    }

    if (!state->eventBuffer || !state->resultBuffer || state->bufferLength == 0) {
        fbd->db->setError(-200161, "Unable to allocate Firebird event buffers");
        FreeEventBuffers(state);
        return false;
    }

    std::memset(state->resultBuffer, 0, state->bufferLength);

    ISC_STATUS_ARRAY status = {};
    ISC_LONG eventId = 0;
    if (isc_que_events(status, state->dbHandle, &eventId, state->bufferLength,
                       reinterpret_cast<const ISC_UCHAR *>(state->eventBuffer),
                       FirebirdEventCallback, state)) {
        SetStatusError(fbd->db, status);
        FreeEventBuffers(state);
        state->eventId = 0;
        return false;
    }

    state->eventId = eventId;
    state->armed = true;
    fbd->db->setError(0, "");
    return true;
}

static void DisposeEventState(FirebirdDbData *fbd) {
    if (!fbd || !fbd->events) return;

    CancelEventListening(fbd);
    FreeEventBuffers(fbd->events);
    delete fbd->events;
    fbd->events = nullptr;
}

struct InsertColumnBinding {
    std::string name;
    std::string value;
    int32_t type = dbTypeNull;
    bool isNull = true;
};

static std::string UppercaseIdentifier(const std::string &value) {
    std::string result = value;
    for (char &ch : result) {
        ch = (char)std::toupper((unsigned char)ch);
    }
    return result;
}

static bool ParseIntegerText(const std::string &text, int64_t &out) {
    if (text.empty()) return false;
    char *endPtr = nullptr;
    long long value = std::strtoll(text.c_str(), &endPtr, 10);
    if (!endPtr || *endPtr != '\0') return false;
    out = (int64_t)value;
    return true;
}

static bool ParseDoubleText(const std::string &text, double &out) {
    if (text.empty()) return false;
    char *endPtr = nullptr;
    double value = std::strtod(text.c_str(), &endPtr);
    if (!endPtr || *endPtr != '\0') return false;
    out = value;
    return true;
}

static bool ParseBooleanText(const std::string &text, bool &out) {
    std::string normalized;
    normalized.reserve(text.size());
    for (char ch : text) {
        if (!std::isspace((unsigned char)ch)) {
            normalized.push_back((char)std::tolower((unsigned char)ch));
        }
    }

    if (normalized == "true" || normalized == "1" || normalized == "yes") {
        out = true;
        return true;
    }
    if (normalized == "false" || normalized == "0" || normalized == "no") {
        out = false;
        return true;
    }
    return false;
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
    cd->signature = kFirebirdCursorSignature;
    cd->db = db;
    cd->stmt = stmt;
    cd->firstRowCalled = false;
    cd->eof = false;
    cd->bof = true;
    cd->allRowsFetched = false;
    cd->currentRow = -1;
    cd->lastValue = nullptr;
    cd->lastType = 0;
    return cd;
}

static bool CacheNextCursorRow(FirebirdCursorData *cd) {
    if (!cd || !cd->stmt || cd->allRowsFetched) return false;

    if (!cd->stmt->fetch()) {
        cd->allRowsFetched = true;
        return false;
    }

    std::vector<FBValue> row;
    const int columnCount = cd->stmt->columnCount();
    row.reserve((size_t)columnCount);
    for (int i = 0; i < columnCount; i++) {
        row.push_back(cd->stmt->columnValue(i));
    }

    cd->rows.push_back(std::move(row));
    return true;
}

static bool EnsureCursorRow(FirebirdCursorData *cd, int rowIndex) {
    if (!cd || rowIndex < 0) return false;

    while ((int)cd->rows.size() <= rowIndex && !cd->allRowsFetched) {
        if (!CacheNextCursorRow(cd)) break;
    }

    return rowIndex >= 0 && rowIndex < (int)cd->rows.size();
}

static void CacheAllCursorRows(FirebirdCursorData *cd) {
    if (!cd) return;
    while (!cd->allRowsFetched) {
        if (!CacheNextCursorRow(cd)) break;
    }
}

static void UpdateCursorFlags(FirebirdCursorData *cd) {
    if (!cd) return;

    const bool empty = cd->allRowsFetched && cd->rows.empty();
    cd->bof = empty || cd->currentRow < 0;
    cd->eof = empty || (cd->allRowsFetched && cd->currentRow >= (int)cd->rows.size());
}

static bool IsMemoryBlockObject(REALobject value) {
    static REALclassRef memoryBlockClass = REALGetClassRef("MemoryBlock");
    return value && memoryBlockClass && REALObjectIsA(value, memoryBlockClass);
}

static bool IsFirebirdBlobObject(REALobject value) {
    static REALclassRef firebirdBlobClass = REALGetClassRef("FirebirdBlob");
    return value && firebirdBlobClass && REALObjectIsA(value, firebirdBlobClass);
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

static void CollectInsertColumns(REALcolumnValue *values, std::vector<InsertColumnBinding> &out) {
    out.clear();

    for (REALcolumnValue *column = values; column != nullptr; column = column->nextColumn) {
        InsertColumnBinding binding;
        binding.name = RealToStd(column->columnName);
        binding.type = column->columnType;
        binding.isNull = (column->columnType == dbTypeNull) || (column->columnValue == nullptr);
        if (!binding.isNull) {
            binding.value = RealToStd(column->columnValue);
        }
        if (!binding.name.empty()) {
            out.push_back(binding);
        }
    }
}

static std::string BuildInsertSQL(const std::string &tableName,
                                  const std::vector<InsertColumnBinding> &columns,
                                  const std::string &returningColumn)
{
    std::ostringstream sql;
    sql << "INSERT INTO " << tableName << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << columns[i].name;
    }
    sql << ") VALUES (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << "?";
    }
    sql << ")";
    if (!returningColumn.empty()) {
        sql << " RETURNING " << returningColumn;
    }
    return sql.str();
}

static bool BindInsertColumn(FBStatement *stmt, FBDatabase *db, int index, const InsertColumnBinding &binding) {
    if (!stmt || !db) return false;

    if (binding.isNull || binding.type == dbTypeNull) {
        stmt->bindNull(index);
        return true;
    }

    switch (binding.type) {
        case dbTypeBoolean: {
            bool boolValue = false;
            if (ParseBooleanText(binding.value, boolValue)) {
                stmt->bindBoolean(index, boolValue);
            } else {
                stmt->bindString(index, binding.value);
            }
            return true;
        }

        case dbTypeByte:
        case dbTypeShort:
        case dbTypeLong:
        case dbTypeInt64:
        case dbTypeUInt64:
        case dbTypeInt8:
        case dbTypeUInt16:
        case dbTypeInt32:
        case dbTypeUInt32: {
            int64_t intValue = 0;
            if (ParseIntegerText(binding.value, intValue)) {
                stmt->bindInt(index, intValue);
            } else {
                stmt->bindString(index, binding.value);
            }
            return true;
        }

        case dbTypeFloat:
        case dbTypeDouble:
        case dbTypeCurrency:
        case dbTypeDecimal: {
            double doubleValue = 0;
            if (ParseDoubleText(binding.value, doubleValue)) {
                stmt->bindDouble(index, doubleValue);
            } else {
                stmt->bindString(index, binding.value);
            }
            return true;
        }

        case dbTypeBinary:
        case dbTypeLongBinary:
            stmt->bindBlob(index, *db, binding.value.data(), binding.value.size());
            return true;

        default:
            stmt->bindString(index, binding.value);
            return true;
    }
}

static bool ResolveReturningColumn(FBDatabase *db,
                                   const std::string &tableName,
                                   const std::string &requestedIdColumn,
                                   std::string &outColumn)
{
    if (!db) return false;

    if (!requestedIdColumn.empty()) {
        outColumn = requestedIdColumn;
        return true;
    }

    FBStatement stmt;
    if (!stmt.prepare(*db, FBDatabase::primaryKeyColumnSQL())) return false;
    stmt.bindString(0, UppercaseIdentifier(tableName));
    if (!stmt.execute(*db)) return false;
    if (!stmt.fetch() || stmt.columnCount() < 1) {
        db->setError(-200101, "Could not determine primary key column for table " + tableName);
        return false;
    }

    FBValue value = stmt.columnValue(0);
    if (value.isNull || value.strVal.empty()) {
        db->setError(-200102, "Primary key column lookup returned an empty value for table " + tableName);
        return false;
    }

    outColumn = value.strVal;
    return true;
}

static bool ExtractReturnedIntegerID(FBStatement &stmt, FBDatabase *db, RBInteger &outId) {
    if (!db) return false;
    if (!stmt.fetch() || stmt.columnCount() < 1) {
        db->setError(-200103, "INSERT ... RETURNING did not return a row");
        return false;
    }

    FBValue value = stmt.columnValue(0);
    if (value.isNull) {
        db->setError(-200104, "Generated key value was NULL");
        return false;
    }

    int64_t idValue = 0;
    switch (value.sqltype) {
        case SQL_SHORT:
        case SQL_LONG:
        case SQL_INT64:
            idValue = value.intVal;
            break;

        case SQL_TEXT:
        case SQL_VARYING:
            if (!ParseIntegerText(value.strVal, idValue)) {
                db->setError(-200105, "Generated key value is not an integer");
                return false;
            }
            break;

        default:
            db->setError(-200106, "Generated key value has an unsupported type");
            return false;
    }

    outId = (RBInteger)idValue;
    return true;
}

static bool ExecuteInsertFromDatabaseRow(dbDatabase *dbData,
                                         REALstring tableName,
                                         REALcolumnValue *values,
                                         REALstring idColumnName,
                                         RBInteger *outId)
{
    auto *fbd = (FirebirdDbData *)dbData;
    if (!fbd || !fbd->db || !fbd->db->isConnected()) return false;

    const std::string table = RealToStd(tableName);
    if (table.empty()) {
        fbd->db->setError(-200100, "Table name is required for AddRow");
        return false;
    }

    std::vector<InsertColumnBinding> columns;
    CollectInsertColumns(values, columns);
    if (columns.empty()) {
        fbd->db->setError(-200107, "No column values were supplied for table " + table);
        return false;
    }

    std::string returningColumn;
    if (outId && !ResolveReturningColumn(fbd->db, table, RealToStd(idColumnName), returningColumn)) {
        return false;
    }

    FBStatement stmt;
    if (!stmt.prepare(*fbd->db, BuildInsertSQL(table, columns, returningColumn))) return false;

    for (int i = 0; i < (int)columns.size(); ++i) {
        BindInsertColumn(&stmt, fbd->db, i, columns[(size_t)i]);
    }

    if (!stmt.execute(*fbd->db, true)) return false;

    if (outId && !ExtractReturnedIntegerID(stmt, fbd->db, *outId)) return false;

    if (fbd->autoCommit && fbd->db->hasActiveTransaction()) {
        if (!fbd->db->commit()) return false;
    }

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
        DisposeEventState(fbd);
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

static void fbEngineAddTableRecord(dbDatabase *dbData, REALstring tableName, REALcolumnValue *values) {
    ExecuteInsertFromDatabaseRow(dbData, tableName, values, nullptr, nullptr);
}

static RBInteger fbEngineAddRowWithReturnValue(dbDatabase *dbData,
                                               REALstring tableName,
                                               REALcolumnValue *values,
                                               REALstring idColumnName)
{
    RBInteger newId = 0;
    if (!ExecuteInsertFromDatabaseRow(dbData, tableName, values, idColumnName, &newId)) {
        return 0;
    }
    return newId;
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
    bool ok = stmt.execute(*fbd->db, true);

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
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt) return 0;
    CacheAllCursorRows(cd);
    return (int)cd->rows.size();
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

    if (cd->currentRow < 0 || cd->currentRow >= (int)cd->rows.size()) {
        *type = (unsigned char)dbTypeNull;
        *value = nullptr;
        *length = 0;
        return;
    }

    if (col < 0 || col >= (int)cd->rows[(size_t)cd->currentRow].size()) {
        *type = (unsigned char)dbTypeNull;
        *value = nullptr;
        *length = 0;
        return;
    }

    const FBValue &val = cd->rows[(size_t)cd->currentRow][(size_t)col];

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

    const int targetRow = cd->currentRow + 1;
    if (EnsureCursorRow(cd, targetRow)) {
        cd->currentRow = targetRow;
        cd->firstRowCalled = true;
        UpdateCursorFlags(cd);
        return true;
    }

    cd->currentRow = (int)cd->rows.size();
    UpdateCursorFlags(cd);
    return false;
}

static void fbCursorPrevRow(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt) return;

    if (cd->rows.empty() && !cd->allRowsFetched) {
        CacheAllCursorRows(cd);
    }

    if (cd->currentRow > 0) {
        cd->currentRow -= 1;
    } else if (cd->currentRow >= (int)cd->rows.size() && !cd->rows.empty()) {
        cd->currentRow = (int)cd->rows.size() - 1;
    } else {
        cd->currentRow = -1;
    }

    UpdateCursorFlags(cd);
}

static void fbCursorFirstRow(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt) return;

    if (EnsureCursorRow(cd, 0)) {
        cd->currentRow = 0;
    } else {
        cd->currentRow = -1;
    }

    UpdateCursorFlags(cd);
}

static void fbCursorLastRow(dbCursor *cursor) {
    auto *cd = (FirebirdCursorData *)cursor;
    if (!cd || !cd->stmt) return;

    CacheAllCursorRows(cd);
    if (cd->rows.empty()) {
        cd->currentRow = -1;
    } else {
        cd->currentRow = (int)cd->rows.size() - 1;
    }

    UpdateCursorFlags(cd);
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
    data->wireCrypt = nullptr;
    data->authClientPlugins = nullptr;
    data->sslMode = -1;
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
    if (data->wireCrypt) REALUnlockString(data->wireCrypt);
    if (data->authClientPlugins) REALUnlockString(data->authClientPlugins);
}

static REALstring fbClassWireCryptGet(REALobject instance, long) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);
    return StdToReal(data ? RealToStd(data->wireCrypt) : "");
}

static void fbClassWireCryptSet(REALobject instance, long, REALstring value) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);
    if (!data) return;

    const std::string wireCrypt = RealToStd(value);
    SetRealStringField(data->wireCrypt, wireCrypt);
    data->sslMode = WireCryptToSSLModeValue(wireCrypt);
}

static RBInteger fbClassSSLModeGet(REALobject instance, long) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);
    if (!data) return -1;

    if (data->sslMode >= 0) return data->sslMode;
    return WireCryptToSSLModeValue(RealToStd(data->wireCrypt));
}

static void fbClassSSLModeSet(REALobject instance, long, RBInteger value) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);
    if (!data) return;

    std::string wireCrypt;
    if (!SSLModeToWireCryptValue(value, wireCrypt)) {
        data->sslMode = (int32_t)value;
        SetRealStringField(data->wireCrypt, "");
        return;
    }

    data->sslMode = (int32_t)value;
    SetRealStringField(data->wireCrypt, wireCrypt);
}

static RBBoolean fbClassConnect(REALobject instance) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);

    std::string host = RealToStd(data->host);
    std::string dbName = RealToStd(data->databaseName);
    std::string user = RealToStd(data->userName);
    std::string pass = RealToStd(data->password);
    std::string charset = RealToStd(data->characterSet);
    std::string role = RealToStd(data->role);
    std::string wireCrypt;
    std::string authClientPlugins = RealToStd(data->authClientPlugins);

    if (!ResolveEffectiveWireCrypt(data, wireCrypt)) return false;

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

    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;

    DisposeEventState(fbd);

    if (!fbd->db->connect(connStr, user, pass, charset, role, data->dialect,
                          host, data->port, dbName, wireCrypt, authClientPlugins)) {
        REALSetDBIsConnected((REALdbDatabase)instance, false);
        return false;
    }

    fbd->autoCommit = true;
    REALSetDBIsConnected((REALdbDatabase)instance, true);

    return true;
}

static void fbClassListen(REALobject instance, REALstring name) {
    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return;

    const std::string eventName = TrimASCII(RealToStd(name));
    if (eventName.empty()) {
        fbd->db->setError(-200162, "Event name cannot be empty");
        return;
    }

    auto *state = EnsureEventState(fbd);
    if (!state) return;

    {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (std::find(state->names.begin(), state->names.end(), eventName) == state->names.end()) {
            state->names.push_back(eventName);
        }
    }

    ArmEventListening(fbd);
}

static void fbClassStopListening(REALobject instance, REALstring name) {
    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return;

    const std::string eventName = TrimASCII(RealToStd(name));
    if (eventName.empty()) {
        fbd->db->setError(-200165, "Event name cannot be empty");
        return;
    }

    auto *state = EnsureEventState(fbd);
    if (!state) return;

    {
        std::lock_guard<std::mutex> lock(state->mutex);
        auto it = std::remove(state->names.begin(), state->names.end(), eventName);
        state->names.erase(it, state->names.end());
    }

    ArmEventListening(fbd);
}

static void fbClassCheckForNotifications(REALobject instance) {
    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return;

    auto *state = fbd->events;
    if (!state) {
        fbd->db->setError(0, "");
        return;
    }

    std::vector<FirebirdPendingNotification> pending;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        pending.swap(state->pending);
    }

    fbd->db->setError(0, "");

    for (const auto &notification : pending) {
        FireReceivedNotificationEvent(instance, notification.name, (RBInteger)notification.count);
    }
}

static void fbClassNotify(REALobject instance, REALstring name) {
    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return;

    if (!fbd->db->isConnected()) {
        fbd->db->setError(-200163, "Database is not connected");
        return;
    }

    const std::string eventName = TrimASCII(RealToStd(name));
    if (eventName.empty()) {
        fbd->db->setError(-200164, "Event name cannot be empty");
        return;
    }

    const std::string sql = "EXECUTE BLOCK AS BEGIN POST_EVENT '" + EscapeSqlLiteral(eventName) + "'; END";
    const bool ok = fbd->db->executeImmediate(sql);
    if (ok && fbd->autoCommit && fbd->db->hasActiveTransaction()) {
        fbd->db->commit();
    }
}

static REALobject fbClassCreateBlob(REALobject instance) {
    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return nullptr;
    if (!fbd->db->isConnected()) {
        fbd->db->setError(-200180, "Database is not connected");
        return nullptr;
    }

    auto *blob = new FBBlob();
    if (!blob->create(*fbd->db)) {
        delete blob;
        return nullptr;
    }

    return NewFirebirdBlobObject(blob);
}

static REALobject fbClassOpenBlob(REALobject instance, REALdbCursor rowset, REALstring column) {
    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return nullptr;

    auto *cd = GetFirebirdCursorData(rowset);
    if (!cd || !cd->stmt || cd->db != fbd->db) {
        fbd->db->setError(-200186, "RowSet does not belong to this FirebirdDatabase instance");
        return nullptr;
    }

    int rowIndex = cd->currentRow;
    if (rowIndex < 0) rowIndex = 0;
    if (!EnsureCursorRow(cd, rowIndex)) {
        fbd->db->setError(-200187, "RowSet is not positioned on a valid row");
        return nullptr;
    }

    const std::string columnName = RealToStd(column);
    const int columnIndex = FindCursorColumnIndex(cd, columnName);
    if (columnIndex < 0) {
        fbd->db->setError(-200188, "Blob column was not found in the RowSet");
        return nullptr;
    }

    const auto &info = cd->stmt->columnInfo(columnIndex);
    if ((info.baseType()) != SQL_BLOB) {
        fbd->db->setError(-200189, "Selected column is not a BLOB");
        return nullptr;
    }

    const auto &row = cd->rows[(size_t)rowIndex];
    const auto &value = row[(size_t)columnIndex];
    if (value.isNull) {
        fbd->db->setError(-200190, "Selected BLOB column is NULL");
        return nullptr;
    }

    auto *blob = new FBBlob();
    if (!blob->open(*fbd->db, value.blobId)) {
        delete blob;
        return nullptr;
    }

    return NewFirebirdBlobObject(blob);
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

static RBInt64 fbClassAffectedRowCount(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return 0;
    return (RBInt64)fbd->db->affectedRowCount();
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

static RBBoolean fbClassBackupDatabase(REALobject instance, REALstring backupFile) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->backupDatabase(RealToStd(backupFile));
}

static RBBoolean fbClassRestoreDatabase(REALobject instance, REALstring backupFile, REALstring targetDatabase, RBBoolean replaceExisting) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->restoreDatabase(RealToStd(backupFile), RealToStd(targetDatabase), replaceExisting != 0);
}

static RBBoolean fbClassDatabaseStatistics(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->databaseStatistics();
}

static RBBoolean fbClassValidateDatabase(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->validateDatabase();
}

static RBBoolean fbClassSweepDatabase(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->sweepDatabase();
}

static RBBoolean fbClassListLimboTransactions(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->listLimboTransactions();
}

static RBBoolean fbClassCommitLimboTransaction(REALobject instance, RBInt64 transactionId) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->commitLimboTransaction((int64_t)transactionId);
}

static RBBoolean fbClassRollbackLimboTransaction(REALobject instance, RBInt64 transactionId) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->rollbackLimboTransaction((int64_t)transactionId);
}

static RBBoolean fbClassSetSweepInterval(REALobject instance, long interval) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->setSweepInterval(interval);
}

static RBBoolean fbClassShutdownDenyNewAttachments(REALobject instance, long timeoutSeconds) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);
    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;

    std::string wireCrypt;
    if (!ResolveEffectiveWireCrypt(data, wireCrypt)) return false;

    FBDatabase controlDb;
    controlDb.configureServiceContext(RealToStd(data->databaseName),
                                      RealToStd(data->userName),
                                      RealToStd(data->password),
                                      RealToStd(data->role),
                                      RealToStd(data->host),
                                      data->port,
                                      wireCrypt,
                                      RealToStd(data->authClientPlugins));

    RBBoolean ok = controlDb.shutdownDenyNewAttachments(timeoutSeconds);
    fbd->db->copyServiceStateFrom(controlDb);
    return ok;
}

static RBBoolean fbClassBringDatabaseOnline(REALobject instance) {
    ClassData(sFirebirdDatabaseClass, instance, FirebirdClassData, data);
    auto *fbd = EnsureFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;

    std::string wireCrypt;
    if (!ResolveEffectiveWireCrypt(data, wireCrypt)) return false;

    FBDatabase controlDb;
    controlDb.configureServiceContext(RealToStd(data->databaseName),
                                      RealToStd(data->userName),
                                      RealToStd(data->password),
                                      RealToStd(data->role),
                                      RealToStd(data->host),
                                      data->port,
                                      wireCrypt,
                                      RealToStd(data->authClientPlugins));

    RBBoolean ok = controlDb.bringDatabaseOnline();
    fbd->db->copyServiceStateFrom(controlDb);
    return ok;
}

static RBBoolean fbClassDisplayUsers(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->displayUsers();
}

static RBBoolean fbClassAddUser(REALobject instance, REALstring userName, REALstring password) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->addUser(RealToStd(userName), RealToStd(password));
}

static RBBoolean fbClassChangeUserPassword(REALobject instance, REALstring userName, REALstring password) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->changeUserPassword(RealToStd(userName), RealToStd(password));
}

static RBBoolean fbClassSetUserAdmin(REALobject instance, REALstring userName, RBBoolean isAdmin) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->setUserAdmin(RealToStd(userName), isAdmin != 0);
}

static RBBoolean fbClassUpdateUserNames(REALobject instance, REALstring userName, REALstring firstName, REALstring middleName, REALstring lastName) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->updateUserNames(RealToStd(userName), RealToStd(firstName), RealToStd(middleName), RealToStd(lastName));
}

static RBBoolean fbClassDeleteUser(REALobject instance, REALstring userName) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return false;
    return fbd->db->deleteUser(RealToStd(userName));
}

static REALstring fbClassLastServiceOutput(REALobject instance) {
    auto *fbd = GetFirebirdDbData(instance);
    if (!fbd || !fbd->db) return StdToReal("");
    return StdToReal(fbd->db->lastServiceOutput());
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

static void fbPrepStmtBindBlob(REALobject instance, int32_t index, REALobject value) {
    ClassData(sFirebirdPreparedStmtClass, instance, FirebirdPreparedStmtData, data);
    if (!data->db || !data->stmt || !value) return;
    if (!IsFirebirdBlobObject(value)) {
        data->db->setError(-200191, "BindBlob expects a FirebirdBlob instance");
        return;
    }

    auto *blobData = (FirebirdBlobData *)REALGetClassData(value, &sFirebirdBlobClass);
    if (!blobData || !blobData->blob) return;
    if (blobData->blob->database() != data->db) {
        data->db->setError(-200192, "Blob belongs to a different FirebirdDatabase connection");
        return;
    }
    if (blobData->blob->isOpen() && !blobData->blob->close()) {
        return;
    }

    data->stmt->bindExistingBlob(index, blobData->blob->blobId());
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
    data->stmt->execute(*data->db, true);
}

// ============================================================================
// FirebirdBlob Class Implementation
// ============================================================================

static void fbBlobConstructor(REALobject instance) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    data->blob = nullptr;
}

static void fbBlobDestructor(REALobject instance) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    if (data->blob) {
        delete data->blob;
        data->blob = nullptr;
    }
}

static REALstring fbBlobIdGet(REALobject instance, long) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    if (!data->blob) return StdToReal("");
    return StdToReal(BlobIdToString(data->blob->blobId()));
}

static RBInt64 fbBlobLengthGet(REALobject instance, long) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    if (!data->blob) return 0;
    return (RBInt64)data->blob->length();
}

static RBInt64 fbBlobPositionGet(REALobject instance, long) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    if (!data->blob) return 0;
    return (RBInt64)data->blob->position();
}

static RBBoolean fbBlobIsOpenGet(REALobject instance, long) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    return (data->blob && data->blob->isOpen()) ? 1 : 0;
}

static REALobject fbBlobRead(REALobject instance, int32_t count) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    if (!data->blob || count < 0) return nullptr;

    std::string bytes;
    if (!data->blob->read((size_t)count, bytes)) return nullptr;

    REALmemoryBlock block = REALNewMemoryBlock((int)bytes.size());
    if (!block) return nullptr;

    void *ptr = REALMemoryBlockGetPtr(block);
    if (ptr && !bytes.empty()) {
        memcpy(ptr, bytes.data(), bytes.size());
    }

    return block;
}

static RBBoolean fbBlobWrite(REALobject instance, REALobject value) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    if (!data->blob || !IsMemoryBlockObject(value)) return false;

    REALmemoryBlock block = (REALmemoryBlock)value;
    void *ptr = REALMemoryBlockGetPtr(block);
    RBInteger size = REALMemoryBlockGetSize(block);
    return data->blob->write(ptr, (size_t)size) ? 1 : 0;
}

static RBInt64 fbBlobSeek(REALobject instance, RBInt64 offset, int32_t whence) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    if (!data->blob) return -1;
    return (RBInt64)data->blob->seek((int64_t)offset, whence);
}

static RBBoolean fbBlobClose(REALobject instance) {
    ClassData(sFirebirdBlobClass, instance, FirebirdBlobData, data);
    if (!data->blob) return true;
    return data->blob->close() ? 1 : 0;
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

    SetClassConsoleSafe(&sFirebirdBlobClass);
    REALRegisterClass(&sFirebirdBlobClass);
}
