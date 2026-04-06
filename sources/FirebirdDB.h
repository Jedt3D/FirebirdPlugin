// FirebirdDB.h — Thin C++ wrapper over libfbclient (Firebird legacy C API)
// This layer handles all direct Firebird interaction so the Xojo plugin
// bridge never touches isc_* functions directly.

#ifndef FIREBIRD_DB_H
#define FIREBIRD_DB_H

#include <ibase.h>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

#ifndef SQLDA_CURRENT_VERSION
#define SQLDA_CURRENT_VERSION SQLDA_VERSION1
#endif

// ---------------------------------------------------------------------------
// Column metadata returned after preparing a statement
// ---------------------------------------------------------------------------
struct FBColumnInfo {
    std::string name;
    std::string alias;
    std::string relation;       // table name
    short       sqltype;        // raw Firebird sqltype (with nullable bit)
    short       sqlsubtype;
    short       sqlscale;
    short       sqllen;

    short baseType() const { return sqltype & ~1; }
    bool  isNullable() const { return (sqltype & 1) != 0; }
};

// ---------------------------------------------------------------------------
// A single fetched value — lightweight, owns its data buffer
// ---------------------------------------------------------------------------
struct FBValue {
    bool        isNull = true;
    short       sqltype = 0;    // base type (no nullable bit)
    short       sqlscale = 0;
    short       sqlsubtype = 0;
    std::string strVal;         // text representation or raw bytes
    int64_t     intVal = 0;
    double      dblVal = 0.0;
    ISC_DATE    dateVal = 0;
    ISC_TIME    timeVal = 0;
    ISC_TIMESTAMP tsVal = {};
    ISC_QUAD    blobId = {};
};

// ---------------------------------------------------------------------------
// FBStatement — wraps a prepared statement and its XSQLDA descriptors
// ---------------------------------------------------------------------------
class FBDatabase;

class FBStatement {
public:
    FBStatement();
    ~FBStatement();

    // Non-copyable
    FBStatement(const FBStatement &) = delete;
    FBStatement &operator=(const FBStatement &) = delete;

    bool prepare(FBDatabase &db, const std::string &sql);
    bool execute(FBDatabase &db);
    bool fetch();                           // returns false when no more rows
    void close();

    int  columnCount() const;
    const FBColumnInfo &columnInfo(int index) const;
    FBValue columnValue(int index) const;   // read current row

    int  paramCount() const;
    short paramBaseType(int index) const;
    short paramScale(int index) const;

    // Bind helpers — index is 0-based
    void bindNull(int index);
    void bindString(int index, const std::string &val);
    void bindInt(int index, int64_t val);
    void bindDouble(int index, double val);
    void bindBlob(int index, FBDatabase &db, const void *data, size_t len);
    void bindDate(int index, ISC_DATE val);
    void bindTime(int index, ISC_TIME val);
    void bindTimestamp(int index, ISC_TIMESTAMP val);
    void bindBoolean(int index, bool val);

    int  statementType() const;             // isc_info_sql_stmt_* value

private:
    void allocateXSQLDA(XSQLDA *&sqlda, int n);
    void allocateBuffers(XSQLDA *sqlda);
    void freeXSQLDA(XSQLDA *&sqlda);
    void buildColumnInfo();
    int  queryStatementType();

    isc_stmt_handle mStmt = 0;
    XSQLDA         *mOutSqlda = nullptr;
    XSQLDA         *mInSqlda = nullptr;
    std::vector<FBColumnInfo> mColumns;
    int             mStmtType = 0;
    bool            mPrepared = false;
    bool            mExecuted = false;
    bool            mExecProcRowPending = false;

    friend class FBDatabase;
};

// ---------------------------------------------------------------------------
// FBDatabase — wraps a Firebird database connection
// ---------------------------------------------------------------------------
class FBDatabase {
public:
    FBDatabase();
    ~FBDatabase();

    // Non-copyable
    FBDatabase(const FBDatabase &) = delete;
    FBDatabase &operator=(const FBDatabase &) = delete;

    // Connection
    bool connect(const std::string &database,
                 const std::string &user,
                 const std::string &password,
                 const std::string &charset = "UTF8",
                 const std::string &role = "",
                 int dialect = 3,
                 const std::string &host = "",
                 int port = 3050,
                 const std::string &databasePath = "");
    void disconnect();
    bool isConnected() const { return mConnected; }

    // Transactions
    bool beginTransaction();
    bool beginTransactionWithOptions(const std::string &isolation, bool readOnly, long lockTimeout);
    bool commit();
    bool rollback();
    bool hasActiveTransaction() const { return mTrans != 0; }

    // Ensure a transaction is active (auto-start if needed)
    bool ensureTransaction();

    // Execute without result set
    bool executeImmediate(const std::string &sql);

    // Read a BLOB into a string buffer (isText=true requests UTF-8 transliteration)
    bool readBlob(ISC_QUAD blobId, std::string &out, bool isText = false);

    // Create a BLOB, returns its ID
    bool writeBlob(const void *data, size_t len, ISC_QUAD &outId);

    // Database information helpers
    bool serverVersion(std::string &out);
    bool pageSize(long &out);
    bool databaseSQLDialect(long &out);
    bool odsVersion(std::string &out);
    bool isReadOnly(bool &out);

    // Transaction information helpers
    bool transactionID(int64_t &out);
    bool transactionIsolation(std::string &out);
    bool transactionAccessMode(std::string &out);
    bool transactionLockTimeout(long &out);

    // Services API helpers
    bool backupDatabase(const std::string &backupFile);
    bool restoreDatabase(const std::string &backupFile, const std::string &targetDatabase, bool replaceExisting);
    bool databaseStatistics();
    const std::string &lastServiceOutput() const { return mServiceOutput; }

    // Error state
    long lastErrorCode() const { return mErrorCode; }
    const std::string &lastErrorString() const { return mErrorMsg; }
    void setError(long code, const std::string &message);

    // Accessors for internal handles (used by FBStatement)
    isc_db_handle  &dbHandle()    { return mDB; }
    isc_tr_handle  &transHandle() { return mTrans; }
    int             dialect() const { return mDialect; }
    const std::string &charset() const { return mCharset; }

    // Schema introspection helpers — return SQL to query Firebird system tables
    static const char *tableListSQL();
    static const char *columnListSQL();       // needs table name parameter
    static const char *indexListSQL();         // needs table name parameter
    static const char *primaryKeyColumnSQL();  // needs table name parameter

private:
    bool databaseInfo(const unsigned char *items, short itemLen, std::vector<char> &out);
    bool transactionInfo(const unsigned char *items, short itemLen, std::vector<char> &out);
    bool runServiceRequest(const std::vector<char> &request, std::string &output);
    const char *findInfoItem(const std::vector<char> &info, unsigned char item, short &len) const;
    void captureError();
    void clearError();

    isc_db_handle   mDB = 0;
    isc_tr_handle   mTrans = 0;
    ISC_STATUS_ARRAY mStatus = {};
    bool            mConnected = false;
    int             mDialect = 3;
    int             mPort = 3050;
    std::string     mCharset = "UTF8";
    std::string     mHost;
    std::string     mDatabasePath;
    std::string     mUser;
    std::string     mPassword;
    std::string     mRole;
    std::string     mServiceOutput;
    long            mErrorCode = 0;
    std::string     mErrorMsg;

    friend class FBStatement;
};

#endif // FIREBIRD_DB_H
