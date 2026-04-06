// ============================================================================
// FirebirdPlugin Quick Reference — API 2.0 Pattern Comparison
// ============================================================================
// Shows side-by-side how FirebirdDatabase follows the same patterns
// as PostgreSQLDatabase and MySQLCommunityServer in Xojo API 2.0.
// ============================================================================


// ---------------------------------------------------------------------------
// CONNECTION PATTERNS (all three follow the same API 2.0 pattern)
// ---------------------------------------------------------------------------

// --- PostgreSQL ---
// Var db As New PostgreSQLDatabase
// db.Host = "localhost"
// db.Port = 5432
// db.DatabaseName = "mydb"
// db.UserName = "postgres"
// db.Password = "secret"
// db.Connect

// --- MySQL ---
// Var db As New MySQLCommunityServer
// db.Host = "localhost"
// db.Port = 3306
// db.DatabaseName = "mydb"
// db.UserName = "root"
// db.Password = "secret"
// db.Connect

// --- Firebird ---
// Var db As New FirebirdDatabase
// db.Host = "localhost"
// db.Port = 3050
// db.DatabaseName = "/path/to/mydb.fdb"
// db.UserName = "SYSDBA"
// db.Password = "masterkey"
// db.CharacterSet = "UTF8"      // Firebird-specific
// db.Role = ""                   // Firebird-specific
// db.Dialect = 3                 // Firebird-specific
// db.Connect


// ---------------------------------------------------------------------------
// QUERY PATTERNS (identical across all database plugins)
// ---------------------------------------------------------------------------

Sub CommonPatterns(db As Database)
  // These work identically whether db is PostgreSQL, MySQL, or Firebird

  // 1. Simple SELECT
  Var rs As RowSet = db.SelectSQL("SELECT * FROM users")

  // 2. Parameterized SELECT
  rs = db.SelectSQL("SELECT * FROM users WHERE age > ? AND city = ?", 25, "Bangkok")

  // 3. Execute (INSERT/UPDATE/DELETE)
  db.ExecuteSQL("INSERT INTO users (name, age) VALUES (?, ?)", "Somchai", 30)

  // 4. RowSet iteration
  While Not rs.AfterLastRow
    Var name As String = rs.Column("name").StringValue
    Var age As Integer = rs.Column("age").IntegerValue
    rs.MoveToNextRow
  Wend
  rs.Close

  // 5. Transactions
  db.BeginTransaction
  db.ExecuteSQL("UPDATE accounts SET balance = balance - 100 WHERE id = 1")
  db.ExecuteSQL("UPDATE accounts SET balance = balance + 100 WHERE id = 2")
  db.CommitTransaction

  // 6. Error handling
  Try
    db.ExecuteSQL("INVALID SQL HERE")
  Catch err As DatabaseException
    System.DebugLog("Error " + err.ErrorNumber.ToString + ": " + err.Message)
  End Try
End Sub


// ---------------------------------------------------------------------------
// FIREBIRD-SPECIFIC PROPERTIES
// ---------------------------------------------------------------------------

// Property       Type      Default   Description
// -------------- --------- --------- ----------------------------------------
// Host           String    ""        Server hostname (empty = embedded/local)
// Port           Integer   3050      TCP port
// DatabaseName   String    ""        Path to .fdb file (required)
// UserName       String    ""        Defaults to "SYSDBA" if empty
// Password       String    ""        Authentication password
// CharacterSet   String    "UTF8"    Connection character set
// Role           String    ""        SQL role name
// Dialect        Integer   3         SQL dialect (1 or 3; always use 3)


// ---------------------------------------------------------------------------
// FIREBIRD-SPECIFIC METHODS
// ---------------------------------------------------------------------------

Sub FirebirdOnlySurface(db As FirebirdDatabase)
  // Database info helpers
  Var version As String = db.ServerVersion
  Var pageSize As Integer = db.PageSize
  Var dialect As Integer = db.DatabaseSQLDialect
  Var ods As String = db.ODSVersion
  Var readOnly As Boolean = db.IsReadOnly

  // Transaction info helpers
  If db.HasActiveTransaction Then
    Var txnId As Int64 = db.TransactionID
    Var isolation As String = db.TransactionIsolation
    Var accessMode As String = db.TransactionAccessMode
    Var lockTimeout As Integer = db.TransactionLockTimeout
  End If

  // Explicit transaction options (Firebird-specific)
  Var started As Boolean = db.BeginTransactionWithOptions("read committed read consistency", True, 0)
  If started Then
    Var explicitIsolation As String = db.TransactionIsolation
    Var explicitAccessMode As String = db.TransactionAccessMode
    Var explicitTimeout As Integer = db.TransactionLockTimeout
    db.RollbackTransaction
  Else
    Var beginError As String = db.ErrorMessage
  End If

  // Services API first slice
  Var backupPath As String = SpecialFolder.Temporary.Child("firebird_quickref.fbk").NativePath
  Var restorePath As String = SpecialFolder.Temporary.Child("firebird_quickref_restore.fdb").NativePath
  Var backupOk As Boolean = db.BackupDatabase(backupPath)
  Var backupLog As String = db.LastServiceOutput
  Var restoreOk As Boolean = db.RestoreDatabase(backupPath, restorePath, True)
  Var restoreLog As String = db.LastServiceOutput
  Var statsOk As Boolean = db.DatabaseStatistics
  Var statsLog As String = db.LastServiceOutput
  Var validateOk As Boolean = db.ValidateDatabase
  Var validateLog As String = db.LastServiceOutput
  Var sweepOk As Boolean = db.SweepDatabase
  Var sweepLog As String = db.LastServiceOutput
  Var limboOk As Boolean = db.ListLimboTransactions
  Var limboLog As String = db.LastServiceOutput
  Var commitLimboOk As Boolean = db.CommitLimboTransaction(12345)
  Var commitLimboLog As String = db.LastServiceOutput
  Var rollbackLimboOk As Boolean = db.RollbackLimboTransaction(12345)
  Var rollbackLimboLog As String = db.LastServiceOutput
  Var setSweepIntervalOk As Boolean = db.SetSweepInterval(20000)
  Var setSweepIntervalLog As String = db.LastServiceOutput
  Var serviceControlDb As New FirebirdDatabase
  serviceControlDb.Host = db.Host
  serviceControlDb.Port = db.Port
  serviceControlDb.DatabaseName = db.DatabaseName
  serviceControlDb.UserName = db.UserName
  serviceControlDb.Password = db.Password
  serviceControlDb.CharacterSet = db.CharacterSet
  serviceControlDb.Role = db.Role
  Var shutdownOk As Boolean = serviceControlDb.ShutdownDenyNewAttachments(1)
  Var shutdownLog As String = serviceControlDb.LastServiceOutput
  Var onlineOk As Boolean = serviceControlDb.BringDatabaseOnline
  Var onlineLog As String = serviceControlDb.LastServiceOutput
  Var usersOk As Boolean = db.DisplayUsers
  Var usersLog As String = db.LastServiceOutput
  Var addUserOk As Boolean = db.AddUser("QUICKREF_USER", "quickref_secret")
  Var addUserLog As String = db.LastServiceOutput
  Var setAdminOk As Boolean = db.SetUserAdmin("QUICKREF_USER", True)
  Var setAdminLog As String = db.LastServiceOutput
  Var updateNamesOk As Boolean = db.UpdateUserNames("QUICKREF_USER", "Quick", "Ref", "User")
  Var updateNamesLog As String = db.LastServiceOutput
  Var changePasswordOk As Boolean = db.ChangeUserPassword("QUICKREF_USER", "quickref_secret_2")
  Var changePasswordLog As String = db.LastServiceOutput
  Var deleteUserOk As Boolean = db.DeleteUser("QUICKREF_USER")
  Var deleteUserLog As String = db.LastServiceOutput

  // Native Xojo DatabaseRow insert convenience
  Var row As New DatabaseRow
  row.Column("Name") = "GeneratedFromAddRow"
  row.Column("Active") = True
  row.Column("Amount") = 12.34
  Var generatedId As Integer = db.AddRow("customers", row, "")

  // Prepared temporal and BLOB binds
  Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement( _
    db.Prepare("INSERT INTO audit_log (event_date, event_time, created_at, note, payload) VALUES (?, ?, ?, ?, ?)"))

  Var stamp As DateTime = DateTime.FromString("2026-04-06 09:30:00")
  ps.Bind(0, stamp)          // DATE
  ps.Bind(1, stamp)          // TIME
  ps.Bind(2, stamp)          // TIMESTAMP
  ps.BindTextBlob(3, "hello")

  Var payloadText As String = "FB" + Chr(0) + Chr(1) + "SQL"
  Var payload As New MemoryBlock(payloadText.Bytes)
  payload.StringValue(0, payloadText.Bytes) = payloadText
  ps.BindBinaryBlob(4, payload)

  // Firebird 4/5/6 modern types use String binds and StringValue readback
  ps = FirebirdPreparedStatement( _
    db.Prepare("INSERT INTO modern_values (v_int128, v_dec34, v_time_tz, v_ts_tz) VALUES (?, ?, ?, ?)"))
  ps.Bind(0, "12345678901234567890123456789012345")
  ps.Bind(1, "12345678901234567890.12345678901234")
  ps.Bind(2, "10:11:12.3456 UTC")
  ps.Bind(3, "2026-04-06 13:14:15.3456 UTC")
  ps.ExecuteSQL

  Var rs As RowSet = db.SelectSQL("SELECT v_int128, v_dec34, v_time_tz, v_ts_tz FROM modern_values")
  If Not rs.AfterLastRow Then
    Var int128Text As String = rs.Column("v_int128").StringValue
    Var dec34Text As String = rs.Column("v_dec34").StringValue
    Var timeTzText As String = rs.Column("v_time_tz").StringValue
    Var tsTzText As String = rs.Column("v_ts_tz").StringValue
  End If
  rs.Close
End Sub


// ---------------------------------------------------------------------------
// FIREBIRD SQL DIFFERENCES FROM MYSQL/POSTGRESQL
// ---------------------------------------------------------------------------

// 1. Auto-increment: Use IDENTITY (Firebird 3+) or SEQUENCE + TRIGGER
//    CREATE TABLE t (id INTEGER GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY)

// 2. String concatenation: Use || operator
//    SELECT first_name || ' ' || last_name FROM users

// 3. LIMIT/OFFSET: Use FIRST/SKIP or ROWS/TO
//    SELECT FIRST 10 SKIP 20 * FROM users
//    -- or --
//    SELECT * FROM users ROWS 21 TO 30

// 4. Boolean: TRUE/FALSE (native BOOLEAN type since Firebird 3.0)
//    SELECT * FROM users WHERE active = TRUE

// 5. RETURNING clause: Get generated values from INSERT
//    INSERT INTO users (name) VALUES ('test') RETURNING id

// 6. EXECUTE BLOCK: Anonymous stored procedures
//    EXECUTE BLOCK AS
//    BEGIN
//      -- procedural code here
//    END

// 7. No IF NOT EXISTS on CREATE TABLE (use EXECUTE BLOCK with exception handling)
//    EXECUTE BLOCK AS
//    BEGIN
//      EXECUTE STATEMENT 'CREATE TABLE ...';
//      WHEN ANY DO BEGIN END  -- ignore if exists
//    END

// 8. BLOB SUB_TYPE TEXT = text BLOB (like MySQL TEXT or PostgreSQL TEXT)
//    CREATE TABLE t (content BLOB SUB_TYPE TEXT)
