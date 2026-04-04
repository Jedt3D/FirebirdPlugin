// FirebirdDB.cpp — Implementation of thin C++ wrapper over libfbclient

#include "FirebirdDB.h"
#include <sstream>

// ============================================================================
// FBDatabase
// ============================================================================

FBDatabase::FBDatabase() {}

FBDatabase::~FBDatabase() {
    disconnect();
}

bool FBDatabase::connect(const std::string &database,
                         const std::string &user,
                         const std::string &password,
                         const std::string &charset,
                         const std::string &role,
                         int dialect)
{
    if (mConnected) disconnect();
    clearError();
    mDialect = dialect;

    // Build DPB
    char dpb[512];
    short dpb_len = 0;

    dpb[dpb_len++] = isc_dpb_version1;

    if (!user.empty()) {
        dpb[dpb_len++] = isc_dpb_user_name;
        dpb[dpb_len++] = (char)user.size();
        memcpy(&dpb[dpb_len], user.c_str(), user.size());
        dpb_len += user.size();
    }

    if (!password.empty()) {
        dpb[dpb_len++] = isc_dpb_password;
        dpb[dpb_len++] = (char)password.size();
        memcpy(&dpb[dpb_len], password.c_str(), password.size());
        dpb_len += password.size();
    }

    if (!charset.empty()) {
        dpb[dpb_len++] = isc_dpb_lc_ctype;
        dpb[dpb_len++] = (char)charset.size();
        memcpy(&dpb[dpb_len], charset.c_str(), charset.size());
        dpb_len += charset.size();
    }

    if (!role.empty()) {
        dpb[dpb_len++] = isc_dpb_sql_role_name;
        dpb[dpb_len++] = (char)role.size();
        memcpy(&dpb[dpb_len], role.c_str(), role.size());
        dpb_len += role.size();
    }

    // SQL dialect
    dpb[dpb_len++] = isc_dpb_sql_dialect;
    dpb[dpb_len++] = 1;
    dpb[dpb_len++] = (char)dialect;

    mDB = 0;
    if (isc_attach_database(mStatus, 0, database.c_str(), &mDB, dpb_len, dpb)) {
        captureError();
        return false;
    }

    mConnected = true;
    return true;
}

void FBDatabase::disconnect() {
    if (!mConnected) return;

    if (mTrans) {
        isc_rollback_transaction(mStatus, &mTrans);
        mTrans = 0;
    }

    isc_detach_database(mStatus, &mDB);
    mDB = 0;
    mConnected = false;
}

bool FBDatabase::beginTransaction() {
    if (!mConnected) return false;
    if (mTrans) return true; // already active
    clearError();

    mTrans = 0;
    if (isc_start_transaction(mStatus, &mTrans, 1, &mDB, 0, nullptr)) {
        captureError();
        return false;
    }
    return true;
}

bool FBDatabase::commit() {
    if (!mTrans) return true;
    clearError();

    if (isc_commit_transaction(mStatus, &mTrans)) {
        captureError();
        return false;
    }
    mTrans = 0;
    return true;
}

bool FBDatabase::rollback() {
    if (!mTrans) return true;
    clearError();

    if (isc_rollback_transaction(mStatus, &mTrans)) {
        captureError();
        return false;
    }
    mTrans = 0;
    return true;
}

bool FBDatabase::ensureTransaction() {
    if (mTrans) return true;
    return beginTransaction();
}

bool FBDatabase::executeImmediate(const std::string &sql) {
    if (!mConnected) return false;
    clearError();

    if (!ensureTransaction()) return false;

    if (isc_dsql_execute_immediate(mStatus, &mDB, &mTrans, 0, sql.c_str(),
                                   mDialect, nullptr)) {
        captureError();
        return false;
    }
    return true;
}

bool FBDatabase::readBlob(ISC_QUAD blobId, std::string &out) {
    if (!mConnected || !mTrans) return false;
    clearError();

    isc_blob_handle blob = 0;
    if (isc_open_blob2(mStatus, &mDB, &mTrans, &blob, &blobId, 0, nullptr)) {
        captureError();
        return false;
    }

    out.clear();
    char buf[4096];
    unsigned short actual = 0;
    ISC_STATUS stat;

    for (;;) {
        stat = isc_get_segment(mStatus, &blob, &actual, sizeof(buf), buf);
        if (stat == 0 || mStatus[1] == isc_segment) {
            out.append(buf, actual);
        } else {
            break;
        }
    }

    isc_close_blob(mStatus, &blob);
    return true;
}

bool FBDatabase::writeBlob(const void *data, size_t len, ISC_QUAD &outId) {
    if (!mConnected || !mTrans) return false;
    clearError();

    isc_blob_handle blob = 0;
    if (isc_create_blob2(mStatus, &mDB, &mTrans, &blob, &outId, 0, nullptr)) {
        captureError();
        return false;
    }

    const char *ptr = (const char *)data;
    size_t remaining = len;
    while (remaining > 0) {
        unsigned short chunk = (remaining > 32767) ? 32767 : (unsigned short)remaining;
        if (isc_put_segment(mStatus, &blob, chunk, ptr)) {
            captureError();
            isc_close_blob(mStatus, &blob);
            return false;
        }
        ptr += chunk;
        remaining -= chunk;
    }

    isc_close_blob(mStatus, &blob);
    return true;
}

void FBDatabase::captureError() {
    char buf[512];
    std::ostringstream oss;
    const ISC_STATUS *pvector = mStatus;

    mErrorCode = isc_sqlcode(mStatus);

    while (fb_interpret(buf, sizeof(buf), &pvector)) {
        if (!oss.str().empty()) oss << "\n";
        oss << buf;
    }
    mErrorMsg = oss.str();
}

void FBDatabase::clearError() {
    mErrorCode = 0;
    mErrorMsg.clear();
    memset(mStatus, 0, sizeof(mStatus));
}

// Schema SQL — queries Firebird system tables
const char *FBDatabase::tableListSQL() {
    return "SELECT TRIM(RDB$RELATION_NAME) AS TABLE_NAME "
           "FROM RDB$RELATIONS "
           "WHERE RDB$SYSTEM_FLAG = 0 AND RDB$VIEW_BLR IS NULL "
           "ORDER BY RDB$RELATION_NAME";
}

const char *FBDatabase::columnListSQL() {
    return "SELECT TRIM(RF.RDB$FIELD_NAME) AS COLUMN_NAME, "
           "F.RDB$FIELD_TYPE AS FIELD_TYPE, "
           "F.RDB$FIELD_SUB_TYPE AS FIELD_SUB_TYPE, "
           "F.RDB$FIELD_LENGTH AS FIELD_LENGTH, "
           "F.RDB$FIELD_PRECISION AS FIELD_PRECISION, "
           "F.RDB$FIELD_SCALE AS FIELD_SCALE, "
           "RF.RDB$NULL_FLAG AS NOT_NULL_FLAG, "
           "RF.RDB$DEFAULT_SOURCE AS DEFAULT_SOURCE, "
           "F.RDB$CHARACTER_LENGTH AS CHAR_LENGTH "
           "FROM RDB$RELATION_FIELDS RF "
           "JOIN RDB$FIELDS F ON RF.RDB$FIELD_SOURCE = F.RDB$FIELD_NAME "
           "WHERE RF.RDB$RELATION_NAME = ? "
           "ORDER BY RF.RDB$FIELD_POSITION";
}

const char *FBDatabase::indexListSQL() {
    return "SELECT TRIM(I.RDB$INDEX_NAME) AS INDEX_NAME, "
           "TRIM(SEG.RDB$FIELD_NAME) AS COLUMN_NAME, "
           "I.RDB$UNIQUE_FLAG AS IS_UNIQUE, "
           "RC.RDB$CONSTRAINT_TYPE AS CONSTRAINT_TYPE "
           "FROM RDB$INDICES I "
           "LEFT JOIN RDB$INDEX_SEGMENTS SEG ON I.RDB$INDEX_NAME = SEG.RDB$INDEX_NAME "
           "LEFT JOIN RDB$RELATION_CONSTRAINTS RC ON I.RDB$INDEX_NAME = RC.RDB$INDEX_NAME "
           "WHERE I.RDB$RELATION_NAME = ? AND I.RDB$SYSTEM_FLAG = 0 "
           "ORDER BY I.RDB$INDEX_NAME, SEG.RDB$FIELD_POSITION";
}

// ============================================================================
// FBStatement
// ============================================================================

FBStatement::FBStatement() {}

FBStatement::~FBStatement() {
    close();
}

void FBStatement::allocateXSQLDA(XSQLDA *&sqlda, int n) {
    if (n < 1) n = 1;
    sqlda = (XSQLDA *)malloc(XSQLDA_LENGTH(n));
    memset(sqlda, 0, XSQLDA_LENGTH(n));
    sqlda->version = SQLDA_CURRENT_VERSION;
    sqlda->sqln = n;
}

void FBStatement::allocateBuffers(XSQLDA *sqlda) {
    if (!sqlda) return;
    for (int i = 0; i < sqlda->sqld; i++) {
        XSQLVAR *var = &sqlda->sqlvar[i];
        short dtype = var->sqltype & ~1;

        switch (dtype) {
            case SQL_VARYING:
                var->sqldata = (char *)calloc(1, var->sqllen + 2);
                break;
            case SQL_TEXT:
                var->sqldata = (char *)calloc(1, var->sqllen + 1);
                break;
            case SQL_SHORT:
                var->sqldata = (char *)calloc(1, sizeof(short));
                break;
            case SQL_LONG:
                var->sqldata = (char *)calloc(1, sizeof(ISC_LONG));
                break;
            case SQL_INT64:
                var->sqldata = (char *)calloc(1, sizeof(ISC_INT64));
                break;
            case SQL_FLOAT:
                var->sqldata = (char *)calloc(1, sizeof(float));
                break;
            case SQL_DOUBLE:
                var->sqldata = (char *)calloc(1, sizeof(double));
                break;
            case SQL_TIMESTAMP:
                var->sqldata = (char *)calloc(1, sizeof(ISC_TIMESTAMP));
                break;
            case SQL_TYPE_DATE:
                var->sqldata = (char *)calloc(1, sizeof(ISC_DATE));
                break;
            case SQL_TYPE_TIME:
                var->sqldata = (char *)calloc(1, sizeof(ISC_TIME));
                break;
            case SQL_BLOB:
                var->sqldata = (char *)calloc(1, sizeof(ISC_QUAD));
                break;
#ifdef SQL_BOOLEAN
            case SQL_BOOLEAN:
                var->sqldata = (char *)calloc(1, sizeof(unsigned char));
                break;
#endif
            default:
                var->sqldata = (char *)calloc(1, var->sqllen > 0 ? var->sqllen : 8);
                break;
        }

        if (var->sqltype & 1) {
            var->sqlind = (short *)calloc(1, sizeof(short));
        }
    }
}

void FBStatement::freeXSQLDA(XSQLDA *&sqlda) {
    if (!sqlda) return;
    for (int i = 0; i < sqlda->sqln; i++) {
        if (sqlda->sqlvar[i].sqldata) free(sqlda->sqlvar[i].sqldata);
        if (sqlda->sqlvar[i].sqlind) free(sqlda->sqlvar[i].sqlind);
    }
    free(sqlda);
    sqlda = nullptr;
}

bool FBStatement::prepare(FBDatabase &db, const std::string &sql) {
    close();
    db.clearError();

    if (!db.ensureTransaction()) return false;

    mStmt = 0;
    if (isc_dsql_allocate_statement(db.mStatus, &db.mDB, &mStmt)) {
        db.captureError();
        return false;
    }

    // Allocate initial output XSQLDA
    allocateXSQLDA(mOutSqlda, 20);

    if (isc_dsql_prepare(db.mStatus, &db.mTrans, &mStmt, 0, sql.c_str(),
                         db.dialect(), mOutSqlda)) {
        db.captureError();
        close();
        return false;
    }

    // Resize if needed
    if (mOutSqlda->sqld > mOutSqlda->sqln) {
        int n = mOutSqlda->sqld;
        freeXSQLDA(mOutSqlda);
        allocateXSQLDA(mOutSqlda, n);
        if (isc_dsql_describe(db.mStatus, &mStmt, 1, mOutSqlda)) {
            db.captureError();
            close();
            return false;
        }
    }

    allocateBuffers(mOutSqlda);
    buildColumnInfo();

    // Describe input parameters
    allocateXSQLDA(mInSqlda, 10);
    if (isc_dsql_describe_bind(db.mStatus, &mStmt, 1, mInSqlda)) {
        db.captureError();
        close();
        return false;
    }

    if (mInSqlda->sqld > mInSqlda->sqln) {
        int n = mInSqlda->sqld;
        freeXSQLDA(mInSqlda);
        allocateXSQLDA(mInSqlda, n);
        if (isc_dsql_describe_bind(db.mStatus, &mStmt, 1, mInSqlda)) {
            db.captureError();
            close();
            return false;
        }
    }

    allocateBuffers(mInSqlda);

    mStmtType = queryStatementType();
    mPrepared = true;
    mExecuted = false;
    return true;
}

int FBStatement::queryStatementType() {
    char type_item[] = { isc_info_sql_stmt_type };
    char info_buffer[20];
    memset(info_buffer, 0, sizeof(info_buffer));

    ISC_STATUS_ARRAY status;
    if (isc_dsql_sql_info(status, &mStmt, sizeof(type_item), type_item,
                          sizeof(info_buffer), info_buffer)) {
        return 0;
    }

    if (info_buffer[0] == isc_info_sql_stmt_type) {
        short len = (short)isc_vax_integer(&info_buffer[1], 2);
        return (int)isc_vax_integer(&info_buffer[3], len);
    }
    return 0;
}

bool FBStatement::execute(FBDatabase &db) {
    if (!mPrepared) return false;
    db.clearError();

    if (!db.ensureTransaction()) return false;

    XSQLDA *inParam = (mInSqlda && mInSqlda->sqld > 0) ? mInSqlda : nullptr;

    if (mStmtType == isc_info_sql_stmt_exec_procedure) {
        if (isc_dsql_execute2(db.mStatus, &db.mTrans, &mStmt, 1, inParam, mOutSqlda)) {
            db.captureError();
            return false;
        }
    } else {
        if (isc_dsql_execute(db.mStatus, &db.mTrans, &mStmt, 1, inParam)) {
            db.captureError();
            return false;
        }
    }

    mExecuted = true;
    return true;
}

bool FBStatement::fetch() {
    if (!mExecuted || !mOutSqlda) return false;

    ISC_STATUS_ARRAY status;
    long stat = isc_dsql_fetch(status, &mStmt, 1, mOutSqlda);
    return (stat == 0);
}

void FBStatement::close() {
    if (mStmt) {
        ISC_STATUS_ARRAY status;
        isc_dsql_free_statement(status, &mStmt, DSQL_drop);
        mStmt = 0;
    }
    freeXSQLDA(mOutSqlda);
    freeXSQLDA(mInSqlda);
    mColumns.clear();
    mPrepared = false;
    mExecuted = false;
    mStmtType = 0;
}

int FBStatement::columnCount() const {
    return mOutSqlda ? mOutSqlda->sqld : 0;
}

const FBColumnInfo &FBStatement::columnInfo(int index) const {
    return mColumns[index];
}

int FBStatement::paramCount() const {
    return mInSqlda ? mInSqlda->sqld : 0;
}

int FBStatement::statementType() const {
    return mStmtType;
}

void FBStatement::buildColumnInfo() {
    mColumns.clear();
    if (!mOutSqlda) return;

    for (int i = 0; i < mOutSqlda->sqld; i++) {
        XSQLVAR *var = &mOutSqlda->sqlvar[i];
        FBColumnInfo ci;

        // Column name: prefer alias, fall back to name
        if (var->aliasname_length > 0) {
            ci.alias.assign(var->aliasname, var->aliasname_length);
            // Trim trailing spaces
            size_t end = ci.alias.find_last_not_of(' ');
            if (end != std::string::npos) ci.alias.resize(end + 1);
        }
        if (var->sqlname_length > 0) {
            ci.name.assign(var->sqlname, var->sqlname_length);
            size_t end = ci.name.find_last_not_of(' ');
            if (end != std::string::npos) ci.name.resize(end + 1);
        }
        if (var->relname_length > 0) {
            ci.relation.assign(var->relname, var->relname_length);
            size_t end = ci.relation.find_last_not_of(' ');
            if (end != std::string::npos) ci.relation.resize(end + 1);
        }

        ci.sqltype = var->sqltype;
        ci.sqlsubtype = var->sqlsubtype;
        ci.sqlscale = var->sqlscale;
        ci.sqllen = var->sqllen;

        mColumns.push_back(ci);
    }
}

FBValue FBStatement::columnValue(int index) const {
    FBValue val;
    if (!mOutSqlda || index < 0 || index >= mOutSqlda->sqld) return val;

    XSQLVAR *var = &mOutSqlda->sqlvar[index];

    // Check NULL
    if ((var->sqltype & 1) && var->sqlind && (*var->sqlind < 0)) {
        val.isNull = true;
        return val;
    }
    val.isNull = false;

    short dtype = var->sqltype & ~1;
    val.sqltype = dtype;
    val.sqlscale = var->sqlscale;
    val.sqlsubtype = var->sqlsubtype;

    switch (dtype) {
        case SQL_TEXT: {
            val.strVal.assign(var->sqldata, var->sqllen);
            // Trim trailing spaces for CHAR fields
            size_t end = val.strVal.find_last_not_of(' ');
            if (end != std::string::npos) val.strVal.resize(end + 1);
            else val.strVal.clear();
            break;
        }
        case SQL_VARYING: {
            short len = *(short *)var->sqldata;
            val.strVal.assign(var->sqldata + sizeof(short), len);
            break;
        }
        case SQL_SHORT: {
            short raw = *(short *)var->sqldata;
            if (var->sqlscale < 0) {
                val.dblVal = (double)raw;
                for (int s = 0; s > var->sqlscale; s--) val.dblVal /= 10.0;
            } else {
                val.intVal = raw;
            }
            break;
        }
        case SQL_LONG: {
            ISC_LONG raw = *(ISC_LONG *)var->sqldata;
            if (var->sqlscale < 0) {
                val.dblVal = (double)raw;
                for (int s = 0; s > var->sqlscale; s--) val.dblVal /= 10.0;
            } else {
                val.intVal = raw;
            }
            break;
        }
        case SQL_INT64: {
            ISC_INT64 raw = *(ISC_INT64 *)var->sqldata;
            if (var->sqlscale < 0) {
                val.dblVal = (double)raw;
                for (int s = 0; s > var->sqlscale; s--) val.dblVal /= 10.0;
            } else {
                val.intVal = (int64_t)raw;
            }
            break;
        }
        case SQL_FLOAT:
            val.dblVal = *(float *)var->sqldata;
            break;
        case SQL_DOUBLE:
            val.dblVal = *(double *)var->sqldata;
            break;
        case SQL_TYPE_DATE:
            val.dateVal = *(ISC_DATE *)var->sqldata;
            break;
        case SQL_TYPE_TIME:
            val.timeVal = *(ISC_TIME *)var->sqldata;
            break;
        case SQL_TIMESTAMP:
            val.tsVal = *(ISC_TIMESTAMP *)var->sqldata;
            break;
        case SQL_BLOB:
            val.blobId = *(ISC_QUAD *)var->sqldata;
            break;
#ifdef SQL_BOOLEAN
        case SQL_BOOLEAN:
            val.intVal = *(unsigned char *)var->sqldata ? 1 : 0;
            break;
#endif
        default:
            // Unknown type — store raw bytes
            val.strVal.assign(var->sqldata, var->sqllen > 0 ? var->sqllen : 0);
            break;
    }

    return val;
}

// ---------------------------------------------------------------------------
// Bind helpers
// ---------------------------------------------------------------------------

void FBStatement::bindNull(int index) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    if (var->sqlind) *var->sqlind = -1;
}

void FBStatement::bindString(int index, const std::string &val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];

    // Reallocate buffer for the string
    short dtype = var->sqltype & ~1;
    if (dtype == SQL_VARYING || dtype == SQL_TEXT) {
        if (var->sqldata) free(var->sqldata);
        if (dtype == SQL_VARYING) {
            var->sqllen = (short)val.size();
            var->sqldata = (char *)calloc(1, val.size() + 2);
            short len = (short)val.size();
            memcpy(var->sqldata, &len, sizeof(short));
            memcpy(var->sqldata + sizeof(short), val.c_str(), val.size());
        } else {
            var->sqldata = (char *)calloc(1, val.size() + 1);
            var->sqllen = (short)val.size();
            memcpy(var->sqldata, val.c_str(), val.size());
        }
    } else {
        // Force VARYING type for text binding to non-text params
        var->sqltype = SQL_VARYING | (var->sqltype & 1);
        if (var->sqldata) free(var->sqldata);
        var->sqllen = (short)val.size();
        var->sqldata = (char *)calloc(1, val.size() + 2);
        short len = (short)val.size();
        memcpy(var->sqldata, &len, sizeof(short));
        memcpy(var->sqldata + sizeof(short), val.c_str(), val.size());
    }

    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindInt(int index, int64_t val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    short dtype = var->sqltype & ~1;

    switch (dtype) {
        case SQL_SHORT:
            *(short *)var->sqldata = (short)val;
            break;
        case SQL_LONG:
            *(ISC_LONG *)var->sqldata = (ISC_LONG)val;
            break;
        case SQL_INT64:
            *(ISC_INT64 *)var->sqldata = (ISC_INT64)val;
            break;
        default:
            // Force INT64
            var->sqltype = SQL_INT64 | (var->sqltype & 1);
            if (var->sqldata) free(var->sqldata);
            var->sqldata = (char *)calloc(1, sizeof(ISC_INT64));
            *(ISC_INT64 *)var->sqldata = (ISC_INT64)val;
            break;
    }
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindDouble(int index, double val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    short dtype = var->sqltype & ~1;

    if (dtype == SQL_FLOAT) {
        *(float *)var->sqldata = (float)val;
    } else if (dtype == SQL_DOUBLE) {
        *(double *)var->sqldata = val;
    } else if (dtype == SQL_SHORT && var->sqlscale < 0) {
        double scaled = val;
        for (int s = 0; s > var->sqlscale; s--) scaled *= 10.0;
        *(short *)var->sqldata = (short)scaled;
    } else if (dtype == SQL_LONG && var->sqlscale < 0) {
        double scaled = val;
        for (int s = 0; s > var->sqlscale; s--) scaled *= 10.0;
        *(ISC_LONG *)var->sqldata = (ISC_LONG)scaled;
    } else if (dtype == SQL_INT64 && var->sqlscale < 0) {
        double scaled = val;
        for (int s = 0; s > var->sqlscale; s--) scaled *= 10.0;
        *(ISC_INT64 *)var->sqldata = (ISC_INT64)scaled;
    } else {
        var->sqltype = SQL_DOUBLE | (var->sqltype & 1);
        if (var->sqldata) free(var->sqldata);
        var->sqldata = (char *)calloc(1, sizeof(double));
        *(double *)var->sqldata = val;
    }
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindBlob(int index, FBDatabase &db, const void *data, size_t len) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];

    ISC_QUAD blobId;
    if (!db.writeBlob(data, len, blobId)) return;

    var->sqltype = SQL_BLOB | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_QUAD));
    *(ISC_QUAD *)var->sqldata = blobId;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindDate(int index, ISC_DATE val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    var->sqltype = SQL_TYPE_DATE | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_DATE));
    *(ISC_DATE *)var->sqldata = val;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindTime(int index, ISC_TIME val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    var->sqltype = SQL_TYPE_TIME | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_TIME));
    *(ISC_TIME *)var->sqldata = val;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindTimestamp(int index, ISC_TIMESTAMP val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];
    var->sqltype = SQL_TIMESTAMP | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(ISC_TIMESTAMP));
    *(ISC_TIMESTAMP *)var->sqldata = val;
    if (var->sqlind) *var->sqlind = 0;
}

void FBStatement::bindBoolean(int index, bool val) {
    if (!mInSqlda || index >= mInSqlda->sqld) return;
    XSQLVAR *var = &mInSqlda->sqlvar[index];

#ifdef SQL_BOOLEAN
    var->sqltype = SQL_BOOLEAN | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(unsigned char));
    *(unsigned char *)var->sqldata = val ? 1 : 0;
#else
    // Fallback: bind as SMALLINT
    var->sqltype = SQL_SHORT | (var->sqltype & 1);
    if (var->sqldata) free(var->sqldata);
    var->sqldata = (char *)calloc(1, sizeof(short));
    *(short *)var->sqldata = val ? 1 : 0;
#endif
    if (var->sqlind) *var->sqlind = 0;
}
