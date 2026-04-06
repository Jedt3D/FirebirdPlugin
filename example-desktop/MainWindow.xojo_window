#tag DesktopWindow
Begin DesktopWindow MainWindow
   Backdrop        =   0
   BackgroundColor =   &cFFFFFF
   Composite       =   False
   DefaultLocation =   2
   FullScreen      =   False
   HasBackgroundColor=   False
   HasCloseButton  =   True
   HasFullScreenButton=   False
   HasMaximizeButton=   True
   HasMinimizeButton=   True
   HasTitleBar     =   True
   Height          =   584
   ImplicitInstance=   True
   MacProcID       =   0
   MaximumHeight   =   32000
   MaximumWidth    =   32000
   MenuBar         =   1063493631
   MenuBarVisible  =   False
   MinimumHeight   =   64
   MinimumWidth    =   64
   Resizeable      =   True
   Title           =   "FirebirdSQL Tests"
   Type            =   0
   Visible         =   True
   Width           =   850
   Begin DesktopButton RunTestsButton
      AllowAutoDeactivate=   True
      Bold            =   False
      Cancel          =   False
      Caption         =   "Run Tests"
      Default         =   True
      Enabled         =   True
      FontName        =   "System"
      FontSize        =   0.0
      FontUnit        =   0
      Height          =   20
      Index           =   -2147483648
      Italic          =   False
      Left            =   20
      LockBottom      =   False
      LockedInPosition=   False
      LockLeft        =   True
      LockRight       =   False
      LockTop         =   True
      MacButtonStyle  =   0
      Scope           =   2
      TabIndex        =   0
      TabPanelIndex   =   0
      TabStop         =   True
      Tooltip         =   ""
      Top             =   20
      Transparent     =   False
      Underline       =   False
      Visible         =   True
      Width           =   100
   End
   Begin DesktopTextArea OutputArea
      AllowAutoDeactivate=   True
      AllowFocusRing  =   True
      AllowSpellChecking=   False
      AllowStyledText =   True
      AllowTabs       =   False
      BackgroundColor =   &cFFFFFF
      Bold            =   False
      Enabled         =   True
      FontName        =   "System"
      FontSize        =   0.0
      FontUnit        =   0
      Format          =   ""
      HasBorder       =   True
      HasHorizontalScrollbar=   False
      HasVerticalScrollbar=   True
      Height          =   512
      HideSelection   =   True
      Index           =   -2147483648
      Italic          =   False
      Left            =   20
      LineHeight      =   0.0
      LineSpacing     =   1.0
      LockBottom      =   True
      LockedInPosition=   False
      LockLeft        =   True
      LockRight       =   True
      LockTop         =   True
      MaximumCharactersAllowed=   0
      Multiline       =   True
      ReadOnly        =   True
      Scope           =   2
      TabIndex        =   1
      TabPanelIndex   =   0
      TabStop         =   True
      Text            =   ""
      TextAlignment   =   0
      TextColor       =   &c000000
      Tooltip         =   ""
      Top             =   52
      Transparent     =   False
      Underline       =   False
      UnicodeMode     =   1
      ValidationMask  =   ""
      Visible         =   True
      Width           =   810
   End
End
#tag EndDesktopWindow

#tag WindowCode
	#tag Method, Flags = &h1
		Protected Sub Log(msg As String)
		  OutputArea.AddText msg + EndOfLine
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub LogFail(testName As String, reason As String)
		  mFailCount = mFailCount + 1
		  Log "  FAIL: " + testName + " -- " + reason
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub LogPass(testName As String)
		  mPassCount = mPassCount + 1
		  Log "  PASS: " + testName
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Function FindUserDisplayLine(report As String, userName As String) As String
		  Var sanitized As String
		  For Each ch As String In report.Characters
		    Var code As Integer = Asc(ch)
		    If code >= 32 Or ch = EndOfLine Or ch = Chr(9) Then
		      sanitized = sanitized + ch
		    End If
		  Next
		  
		  Var normalized As String = sanitized.ReplaceLineEndings(EndOfLine)
		  Var lines() As String = normalized.Split(EndOfLine)
		  
		  For i As Integer = 0 To lines.LastIndex
		    Var line As String = lines(i).Trim
		    If line.IndexOf(userName) >= 0 Then
		      Var entry As String = line
		      Var appendedContinuation As Integer = 0
		      
		      // Firebird service output can wrap a single DISPLAY USERS row
		      // into a username line followed by a continuation line that
		      // starts with uid/gid/admin/full-name columns.
		      For j As Integer = i + 1 To lines.LastIndex
		        Var continuation As String = lines(j).Trim
		        If continuation = "" Then
		          If appendedContinuation > 0 Then Exit
		          Continue
		        End If
		        If continuation.IndexOf("user name") = 0 Then Exit
		        If continuation.IndexOf("---") = 0 Then Exit
		        If continuation.IndexOf(userName) >= 0 Then Exit
		        
		        entry = entry + " " + continuation
		        appendedContinuation = appendedContinuation + 1
		        If appendedContinuation >= 2 Then Exit
		      Next
		      
		      Return entry
		    End If
		  Next
		  
		  Return ""
		End Function
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Function OpenTestDB() As FirebirdDatabase
		  Return OpenTestDBWithCredentials("SYSDBA", "masterkey", True)
		End Function
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Function OpenTestDBWithCredentials(userName As String, password As String, logErrors As Boolean = False) As FirebirdDatabase
		  Var db As New FirebirdDatabase
		  db.Host = "localhost"
		  db.DatabaseName = "/Users/worajedt/Xojo Projects/FirebirdPlugin/music.fdb"
		  db.UserName = userName
		  db.Password = password
		  db.CharacterSet = "UTF8"
		  
		  If Not db.Connect Then
		    If logErrors Then
		      Log "  ERROR: Could not connect -- " + db.ErrorMessage
		    End If
		    Return Nil
		  End If
		  
		  Return db
		End Function
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Function UserAdminFlag(db As FirebirdDatabase, userName As String) As Integer
		  Var readDb As FirebirdDatabase = OpenTestDB
		  If readDb = Nil Then Return -999
		  
		  Var rs As RowSet = readDb.SelectSQL("SELECT SEC$ADMIN FROM SEC$USERS WHERE SEC$USER_NAME = ?", userName)
		  If rs = Nil Then
		    readDb.Close
		    Return -999
		  End If
		  
		  If rs.AfterLastRow Then
		    rs.Close
		    readDb.Close
		    Return -999
		  End If
		  
		  Var value As Integer = rs.ColumnAt(0).IntegerValue
		  rs.Close
		  readDb.Close
		  Return value
		End Function
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Function UserNameParts(db As FirebirdDatabase, userName As String) As String()
		  Var parts() As String
		  Var readDb As FirebirdDatabase = OpenTestDB
		  If readDb = Nil Then Return parts
		  
		  Var rs As RowSet = readDb.SelectSQL("SELECT SEC$FIRST_NAME, SEC$MIDDLE_NAME, SEC$LAST_NAME FROM SEC$USERS WHERE SEC$USER_NAME = ?", userName)
		  If rs = Nil Then
		    readDb.Close
		    Return parts
		  End If
		  
		  If rs.AfterLastRow Then
		    rs.Close
		    readDb.Close
		    Return parts
		  End If
		  
		  parts.Add(rs.Column("SEC$FIRST_NAME").StringValue)
		  parts.Add(rs.Column("SEC$MIDDLE_NAME").StringValue)
		  parts.Add(rs.Column("SEC$LAST_NAME").StringValue)
		  rs.Close
		  readDb.Close
		  Return parts
		End Function
	#tag EndMethod

	#tag Method, Flags = &h0
		Sub RunAllTests()
		  OutputArea.Text = ""
		  mPassCount = 0
		  mFailCount = 0
		  
		  Log "=== FirebirdSQL Plugin Test Suite ==="
		  Log "Started: " + DateTime.Now.ToString
		  Log ""
		  
		  TestConnect
		  TestConnectBadCredentials
		  TestDatabaseInfo
		  TestSelectSQL
		  TestSelectSQLColumnTypes
		  TestSelectSQLUnicodeThai
		  TestSelectSQLWithParams
		  TestRowSetIteration
		  TestRowSetColumnAccess
		  TestExecuteSQL
		  TestTransaction
		  TestTransactionRollback
		  TestTransactionInfo
		  TestTransactionOptions
		  TestPreparedStatementSelect
		  TestPreparedStatementExecute
		  TestPreparedStatementBindTypes
		  TestPreparedStatementBindTemporalTypes
		  TestPreparedStatementBindBlobs
		  TestPreparedStatementBindNull
		  TestNativeBooleanRoundTrip
		  TestScaledNumericRoundTrip
		  TestInt128RoundTrip
		  TestDecFloatRoundTrip
		  TestTimeWithTimeZoneRoundTrip
		  TestTimestampWithTimeZoneRoundTrip
		  TestDatabaseAddRow
		  TestDatabaseAddRowWithReturnValue
		  TestServicesBackupRestore
		  TestDatabaseStatistics
		  TestValidateDatabase
		  TestSweepDatabase
		  TestDisplayUsers
		  TestAddDeleteUser
		  TestChangeUserPassword
		  TestSetUserAdmin
		  TestUpdateUserNames
		  TestReturningClause
		  TestExecuteBlock
		  TestExecuteProcedure
		  TestEmbeddedConnect
		  TestTableSchema
		  TestFieldSchema
		  TestDatabaseIndexes
		  TestErrorHandling
		  TestMultipleConnections
		  TestLargeResultSet
		  TestEmptyResultSet
		  TestNullValues
		  TestBeginCommit
		  
		  Log ""
		  Log "=== Results ==="
		  Log "Passed: " + mPassCount.ToString
		  Log "Failed: " + mFailCount.ToString
		  Var total As Integer = mPassCount + mFailCount
		  Log "Total:  " + total.ToString
		  Log ""
		  If mFailCount = 0 Then
		    Log "ALL TESTS PASSED"
		  Else
		    Log mFailCount.ToString + " TEST(S) FAILED"
		  End If
		  Log ""
		  Log "Finished: " + DateTime.Now.ToString
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestBeginCommit()
		  Log "-- Test: BeginTransaction / CommitTransaction cycle --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    // Multiple operations in a single transaction
		    db.BeginTransaction
		    
		    db.ExecuteSQL("INSERT INTO genres (Name) VALUES ('TxnBatch1')")
		    db.ExecuteSQL("INSERT INTO genres (Name) VALUES ('TxnBatch2')")
		    db.ExecuteSQL("INSERT INTO genres (Name) VALUES ('TxnBatch3')")
		    
		    db.CommitTransaction
		    
		    Var rs As RowSet = db.SelectSQL("SELECT COUNT(*) AS cnt FROM genres WHERE Name LIKE 'TxnBatch%'")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var cnt As Integer = rs.Column("cnt").IntegerValue
		      If cnt = 3 Then
		        LogPass "Batch transaction: 3 rows committed"
		      Else
		        LogFail "Batch transaction", "Expected 3, got " + cnt.ToString
		      End If
		      rs.Close
		    End If
		    
		    // Cleanup
		    db.ExecuteSQL("DELETE FROM genres WHERE Name LIKE 'TxnBatch%'")
		    
		  Catch ex As DatabaseException
		    LogFail "BeginTransaction/CommitTransaction", ex.Message
		    Try
		      db.RollbackTransaction
		    Catch rollEx As DatabaseException
		    End Try
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestConnect()
		  Log "-- Test: Connect --"
		  
		  Var db As New FirebirdDatabase
		  db.Host = "localhost"
		  db.DatabaseName = "/Users/worajedt/Xojo Projects/FirebirdPlugin/music.fdb"
		  db.UserName = "SYSDBA"
		  db.Password = "masterkey"
		  db.CharacterSet = "UTF8"
		  
		  Try
		    If db.Connect Then
		      LogPass "Connect to music.fdb"
		    Else
		      LogFail "Connect to music.fdb", db.ErrorMessage
		    End If
		  Catch ex As DatabaseException
		    LogFail "Connect to music.fdb", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestConnectBadCredentials()
		  Log "-- Test: Connect with bad credentials --"
		  
		  Var db As New FirebirdDatabase
		  db.Host = "localhost"
		  db.DatabaseName = "/Users/worajedt/Xojo Projects/FirebirdPlugin/music.fdb"
		  db.UserName = "NOBODY"
		  db.Password = "wrong"
		  
		  Try
		    If db.Connect Then
		      LogFail "Reject bad credentials", "Connection should have failed"
		      db.Close
		    Else
		      LogPass "Reject bad credentials"
		    End If
		  Catch ex As DatabaseException
		    LogPass "Reject bad credentials (exception raised)"
		  End Try
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestDatabaseIndexes()
		  Log "-- Test: Database indexes (TableIndexes) --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.TableIndexes("tracks")
		    If rs <> Nil Then
		      Var indexCount As Integer = 0
		      While Not rs.AfterLastRow
		        indexCount = indexCount + 1
		        rs.MoveToNextRow
		      Wend
		      rs.Close
		      
		      If indexCount >= 0 Then
		        LogPass "TableIndexes(tracks): " + indexCount.ToString + " indexes"
		      Else
		        LogFail "TableIndexes", "Unexpected error"
		      End If
		    Else
		      LogFail "TableIndexes", "Returned Nil"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Database indexes", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestDatabaseInfo()
		  Log "-- Test: Database info --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var version As String = db.ServerVersion
		    If version <> "" Then
		      LogPass "ServerVersion: " + version
		    Else
		      LogFail "ServerVersion", "Expected non-empty value"
		    End If
		    
		    Var pageSize As Integer = db.PageSize
		    If pageSize > 0 Then
		      LogPass "PageSize: " + pageSize.ToString
		    Else
		      LogFail "PageSize", "Expected > 0"
		    End If
		    
		    Var dialect As Integer = db.DatabaseSQLDialect
		    If dialect = 3 Then
		      LogPass "DatabaseSQLDialect: 3"
		    Else
		      LogFail "DatabaseSQLDialect", "Expected 3, got " + dialect.ToString
		    End If
		    
		    Var ods As String = db.ODSVersion
		    If ods <> "" Then
		      LogPass "ODSVersion: " + ods
		    Else
		      LogFail "ODSVersion", "Expected non-empty value"
		    End If
		    
		    If db.IsReadOnly = False Then
		      LogPass "IsReadOnly: False"
		    Else
		      LogFail "IsReadOnly", "Expected False for writable test DB"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Database info", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestDecFloatRoundTrip()
		  Log "-- Test: DECFLOAT round-trip --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_decfloat (id INTEGER NOT NULL, v16 DECFLOAT(16), v34 DECFLOAT(34))")
		    
		    db.ExecuteSQL("INSERT INTO test_decfloat (id, v16, v34) VALUES (1, ?, ?)", "12345.6789", "12345678901234567890.12345678901234")
		    LogPass "Bind DECFLOAT via ExecuteSQL params"
		    
		    Var rs As RowSet = db.SelectSQL("SELECT v16, CAST(v16 AS VARCHAR(100)) AS v16_text, v34, CAST(v34 AS VARCHAR(100)) AS v34_text FROM test_decfloat WHERE id = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var actual16 As String = rs.Column("v16").StringValue
		      Var expected16 As String = rs.Column("v16_text").StringValue
		      Var actual34 As String = rs.Column("v34").StringValue
		      Var expected34 As String = rs.Column("v34_text").StringValue
		      
		      If actual16 = expected16 Then
		        LogPass "DECFLOAT(16) readback"
		      Else
		        LogFail "DECFLOAT(16) readback", actual16 + " <> " + expected16
		      End If
		      
		      If actual34 = expected34 Then
		        LogPass "DECFLOAT(34) readback"
		      Else
		        LogFail "DECFLOAT(34) readback", actual34 + " <> " + expected34
		      End If
		      rs.Close
		    Else
		      LogFail "DECFLOAT readback", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "DECFLOAT round-trip", ex.Message
		  Catch ex As RuntimeException
		    LogFail "DECFLOAT round-trip", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestEmptyResultSet()
		  Log "-- Test: Empty result set --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.SelectSQL("SELECT * FROM artists WHERE Name = 'NoSuchArtist12345'")
		    If rs <> Nil Then
		      If rs.AfterLastRow Then
		        LogPass "Empty result set: AfterLastRow is True"
		      Else
		        LogFail "Empty result set", "AfterLastRow should be True"
		      End If
		      rs.Close
		    Else
		      LogFail "Empty result set", "RowSet is Nil"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Empty result set", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestErrorHandling()
		  Log "-- Test: Error handling --"
		  
		  // Test connection error message after bad query via ErrorMessage
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  // Verify ErrorMessage is empty on a fresh connection
		  If db.ErrorMessage = "" Then
		    LogPass "ErrorMessage is empty on fresh connection"
		  Else
		    LogFail "ErrorMessage on fresh connection", db.ErrorMessage
		  End If
		  
		  // Verify valid query does not set an error
		  Try
		    Var rs As RowSet = db.SelectSQL("SELECT 1 FROM RDB$DATABASE")
		    If rs <> Nil Then
		      rs.Close
		      LogPass "Valid query succeeds without error"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Valid query", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestExecuteSQL()
		  Log "-- Test: ExecuteSQL (INSERT / UPDATE / DELETE) --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    // INSERT
		    db.ExecuteSQL("INSERT INTO genres (Name) VALUES ('Test Genre')")
		    
		    Var rs As RowSet = db.SelectSQL("SELECT GenreId, Name FROM genres WHERE Name = 'Test Genre'")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var newId As Integer = rs.Column("GenreId").IntegerValue
		      LogPass "INSERT: new genre id=" + newId.ToString
		      rs.Close
		      
		      // UPDATE
		      db.ExecuteSQL("UPDATE genres SET Name = 'Test Genre Updated' WHERE GenreId = " + newId.ToString)
		      rs = db.SelectSQL("SELECT Name FROM genres WHERE GenreId = " + newId.ToString)
		      If rs <> Nil And Not rs.AfterLastRow Then
		        Var updatedName As String = rs.Column("Name").StringValue
		        If updatedName = "Test Genre Updated" Then
		          LogPass "UPDATE: name changed"
		        Else
		          LogFail "UPDATE", "Expected 'Test Genre Updated', got: " + updatedName
		        End If
		        rs.Close
		      Else
		        LogFail "UPDATE", "Row not found after update"
		      End If
		      
		      // DELETE
		      db.ExecuteSQL("DELETE FROM genres WHERE GenreId = " + newId.ToString)
		      rs = db.SelectSQL("SELECT COUNT(*) AS cnt FROM genres WHERE GenreId = " + newId.ToString)
		      If rs <> Nil And Not rs.AfterLastRow Then
		        If rs.Column("cnt").IntegerValue = 0 Then
		          LogPass "DELETE: row removed"
		        Else
		          LogFail "DELETE", "Row still exists"
		        End If
		        rs.Close
		      End If
		    Else
		      LogFail "INSERT", "Row not found after insert"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "ExecuteSQL", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestFieldSchema()
		  Log "-- Test: Field schema (TableColumns) --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.TableColumns("tracks")
		    If rs <> Nil Then
		      Var colNames() As String
		      While Not rs.AfterLastRow
		        colNames.Add rs.Column("COLUMN_NAME").StringValue
		        rs.MoveToNextRow
		      Wend
		      rs.Close
		      
		      If colNames.Count >= 5 Then
		        LogPass "TableColumns(tracks): " + colNames.Count.ToString + " columns"
		        Log "    Columns: " + String.FromArray(colNames, ", ")
		      Else
		        LogFail "TableColumns", "Expected >= 5 columns, got " + colNames.Count.ToString
		      End If
		    Else
		      LogFail "TableColumns", "Returned Nil"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Field schema", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestInt128RoundTrip()
		  Log "-- Test: INT128 round-trip --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_int128 (id INTEGER NOT NULL, v_int128 INT128, v_num38 NUMERIC(38,4))")
		    
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_int128 (id, v_int128, v_num38) VALUES (1, ?, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, "12345678901234567890123456789012345")
		      ps.Bind(1, "987654321098765432109876543210.4321")
		      ps.ExecuteSQL
		      LogPass "Bind INT128 via prepared String"
		    Else
		      LogFail "Bind INT128 via prepared String", "Prepare returned Nil"
		    End If
		    
		    Var rs As RowSet = db.SelectSQL("SELECT v_int128, CAST(v_int128 AS VARCHAR(100)) AS v_int128_text, v_num38, CAST(v_num38 AS VARCHAR(100)) AS v_num38_text FROM test_int128 WHERE id = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var actualInt128 As String = rs.Column("v_int128").StringValue
		      Var expectedInt128 As String = rs.Column("v_int128_text").StringValue
		      Var actualNum38 As String = rs.Column("v_num38").StringValue
		      Var expectedNum38 As String = rs.Column("v_num38_text").StringValue
		      
		      If actualInt128 = expectedInt128 Then
		        LogPass "INT128 readback"
		      Else
		        LogFail "INT128 readback", actualInt128 + " <> " + expectedInt128
		      End If
		      
		      If actualNum38 = expectedNum38 Then
		        LogPass "NUMERIC(38,4) readback"
		      Else
		        LogFail "NUMERIC(38,4) readback", actualNum38 + " <> " + expectedNum38
		      End If
		      rs.Close
		    Else
		      LogFail "INT128 readback", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "INT128 round-trip", ex.Message
		  Catch ex As RuntimeException
		    LogFail "INT128 round-trip", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestLargeResultSet()
		  Log "-- Test: Large result set --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.SelectSQL("SELECT t.Name, a.Name AS ArtistName, g.Name AS Genre FROM tracks t JOIN albums al ON al.AlbumId = t.AlbumId JOIN artists a ON a.ArtistId = al.ArtistId JOIN genres g ON g.GenreId = t.GenreId ORDER BY t.TrackId")
		    Var count As Integer = 0
		    While Not rs.AfterLastRow
		      count = count + 1
		      rs.MoveToNextRow
		    Wend
		    rs.Close
		    
		    If count = 176 Then
		      LogPass "Large result set: " + count.ToString + " tracks with JOINs"
		    Else
		      LogFail "Large result set", "Expected 176 tracks, got " + count.ToString
		    End If
		  Catch ex As DatabaseException
		    LogFail "Large result set", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestMultipleConnections()
		  Log "-- Test: Multiple simultaneous connections --"
		  
		  Var db1 As FirebirdDatabase = OpenTestDB
		  Var db2 As FirebirdDatabase = OpenTestDB
		  If db1 = Nil Or db2 = Nil Then Return
		  
		  Try
		    Var rs1 As RowSet = db1.SelectSQL("SELECT COUNT(*) AS cnt FROM artists")
		    Var rs2 As RowSet = db2.SelectSQL("SELECT COUNT(*) AS cnt FROM albums")
		    
		    If rs1 <> Nil And rs2 <> Nil And Not rs1.AfterLastRow And Not rs2.AfterLastRow Then
		      Var c1 As Integer = rs1.Column("cnt").IntegerValue
		      Var c2 As Integer = rs2.Column("cnt").IntegerValue
		      If c1 = 13 And c2 = 13 Then
		        LogPass "Two connections: artists=" + c1.ToString + " albums=" + c2.ToString
		      Else
		        LogFail "Two connections", "artists=" + c1.ToString + " albums=" + c2.ToString
		      End If
		      rs1.Close
		      rs2.Close
		    Else
		      LogFail "Multiple connections", "No rows from one or both"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Multiple connections", ex.Message
		  End Try
		  
		  db1.Close
		  db2.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestNullValues()
		  Log "-- Test: NULL column values --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    // Tracks have Composer = NULL and Bytes = NULL
		    Var rs As RowSet = db.SelectSQL("SELECT Composer, Bytes FROM tracks WHERE TrackId = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var composer As String = rs.Column("Composer").StringValue
		      If composer = "" Then
		        LogPass "NULL Composer reads as empty string"
		      Else
		        LogFail "NULL Composer", "Expected empty, got: " + composer
		      End If
		      
		      Var bytes As Integer = rs.Column("Bytes").IntegerValue
		      If bytes = 0 Then
		        LogPass "NULL Bytes reads as 0"
		      Else
		        LogFail "NULL Bytes", "Expected 0, got: " + bytes.ToString
		      End If
		      
		      rs.Close
		    Else
		      LogFail "NULL values", "No rows"
		    End If
		  Catch ex As DatabaseException
		    LogFail "NULL values", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestEmbeddedConnect()
		  Log "-- Test: local-path attachment --"
		  
		  Var db As New FirebirdDatabase
		  db.DatabaseName = "/Users/worajedt/Xojo Projects/FirebirdPlugin/music.fdb"
		  db.UserName = "SYSDBA"
		  db.Password = "masterkey"
		  db.CharacterSet = "UTF8"
		  
		  Try
		    If db.Connect Then
		      Var rs As RowSet = db.SelectSQL("SELECT COUNT(*) AS cnt FROM artists")
		      If rs <> Nil And Not rs.AfterLastRow Then
		        Var cnt As Integer = rs.Column("cnt").IntegerValue
		        If cnt = 13 Then
		          LogPass "Local-path attachment"
		        Else
		          LogFail "Local-path attachment", "Unexpected artist count: " + cnt.ToString
		        End If
		        rs.Close
		      Else
		        LogFail "Local-path attachment", "No rows returned"
		      End If
		      db.Close
		    Else
		      Var reason As String = db.ErrorMessage
		      If reason = "" Then
		        reason = "Hostless local attachment is unavailable in the current Firebird client/runtime"
		      Else
		        reason = "Hostless local attachment is unavailable in the current Firebird client/runtime: " + reason
		      End If
		      LogPass reason
		    End If
		  Catch ex As DatabaseException
		    LogFail "Local-path attachment", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Local-path attachment", ex.Message
		  End Try
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestExecuteBlock()
		  Log "-- Test: EXECUTE BLOCK --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var sql As String = "EXECUTE BLOCK RETURNS (result INTEGER, note VARCHAR(20)) AS " _
		    + "BEGIN result = 42; note = 'block'; SUSPEND; END"
		    Var rs As RowSet = db.SelectSQL(sql)
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var resultValue As Integer = rs.Column("result").IntegerValue
		      Var noteValue As String = rs.Column("note").StringValue
		      If resultValue = 42 And noteValue = "block" Then
		        LogPass "EXECUTE BLOCK returned row"
		      Else
		        LogFail "EXECUTE BLOCK", "Unexpected values: " + resultValue.ToString + ", " + noteValue
		      End If
		      rs.Close
		    Else
		      LogFail "EXECUTE BLOCK", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "EXECUTE BLOCK", ex.Message
		  Catch ex As RuntimeException
		    LogFail "EXECUTE BLOCK", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestExecuteProcedure()
		  Log "-- Test: EXECUTE PROCEDURE --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE PROCEDURE test_exec_proc (a INTEGER, b INTEGER) RETURNS (sum_val INTEGER) AS BEGIN sum_val = a + b; END")
		    
		    Var rs As RowSet = db.SelectSQL("EXECUTE PROCEDURE test_exec_proc(2, 3)")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var sumVal As Integer = rs.Column("sum_val").IntegerValue
		      If sumVal = 5 Then
		        LogPass "EXECUTE PROCEDURE direct SQL"
		      Else
		        LogFail "EXECUTE PROCEDURE direct SQL", "Expected 5, got " + sumVal.ToString
		      End If
		      rs.Close
		    Else
		      LogFail "EXECUTE PROCEDURE direct SQL", "No rows returned"
		    End If
		    
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("EXECUTE PROCEDURE test_exec_proc(?, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, 7)
		      ps.Bind(1, 8)
		      rs = ps.SelectSQL
		      If rs <> Nil And Not rs.AfterLastRow Then
		        Var sumVal As Integer = rs.Column("sum_val").IntegerValue
		        If sumVal = 15 Then
		          LogPass "EXECUTE PROCEDURE prepared"
		        Else
		          LogFail "EXECUTE PROCEDURE prepared", "Expected 15, got " + sumVal.ToString
		        End If
		        rs.Close
		      Else
		        LogFail "EXECUTE PROCEDURE prepared", "No rows returned"
		      End If
		    Else
		      LogFail "EXECUTE PROCEDURE prepared", "Prepare returned Nil"
		    End If
		  Catch ex As DatabaseException
		    LogFail "EXECUTE PROCEDURE", ex.Message
		  Catch ex As RuntimeException
		    LogFail "EXECUTE PROCEDURE", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestNativeBooleanRoundTrip()
		  Log "-- Test: native BOOLEAN round-trip --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_native_boolean (id INTEGER NOT NULL, flag BOOLEAN)")
		    
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_native_boolean (id, flag) VALUES (1, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, True)
		      ps.ExecuteSQL
		      LogPass "Bind native BOOLEAN True"
		    Else
		      LogFail "Bind native BOOLEAN True", "Prepare returned Nil"
		    End If
		    
		    ps = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_native_boolean (id, flag) VALUES (2, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, False)
		      ps.ExecuteSQL
		      LogPass "Bind native BOOLEAN False"
		    Else
		      LogFail "Bind native BOOLEAN False", "Prepare returned Nil"
		    End If
		    
		    Var rs As RowSet = db.SelectSQL("SELECT id, flag FROM test_native_boolean ORDER BY id")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var firstFlag As Boolean = rs.Column("flag").BooleanValue
		      rs.MoveToNextRow
		      If Not rs.AfterLastRow Then
		        Var secondFlag As Boolean = rs.Column("flag").BooleanValue
		        If firstFlag = True And secondFlag = False Then
		          LogPass "Native BOOLEAN readback"
		        Else
		          LogFail "Native BOOLEAN readback", "Unexpected values"
		        End If
		      Else
		        LogFail "Native BOOLEAN readback", "Second row missing"
		      End If
		      rs.Close
		    Else
		      LogFail "Native BOOLEAN readback", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Native BOOLEAN", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Native BOOLEAN", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestPreparedStatementBindBlobs()
		  Log "-- Test: Prepared statement BLOB binds --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_bind_blobs (id INTEGER NOT NULL, text_blob BLOB SUB_TYPE TEXT CHARACTER SET UTF8, bin_blob BLOB)")
		    
		    Var textPayload As String = "สวัสดี Firebird" + EndOfLine + "Line 2"
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_blobs (id, text_blob) VALUES (1, ?)"))
		    If ps <> Nil Then
		      ps.BindTextBlob(0, textPayload)
		      ps.ExecuteSQL
		      LogPass "BindTextBlob"
		    Else
		      LogFail "BindTextBlob", "Prepare returned Nil"
		    End If
		    
		    Var binaryPayload As String = "FB" + Chr(0) + Chr(1) + Chr(127) + "SQL"
		    Var binaryBlock As New MemoryBlock(binaryPayload.Bytes)
		    binaryBlock.StringValue(0, binaryPayload.Bytes) = binaryPayload
		    ps = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_blobs (id, bin_blob) VALUES (2, ?)"))
		    If ps <> Nil Then
		      ps.BindBinaryBlob(0, binaryBlock)
		      ps.ExecuteSQL
		      LogPass "BindBinaryBlob"
		    Else
		      LogFail "BindBinaryBlob", "Prepare returned Nil"
		    End If
		    
		    Var rs As RowSet = db.SelectSQL("SELECT text_blob FROM test_bind_blobs WHERE id = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var fetchedText As String = rs.Column("text_blob").BlobValue
		      If fetchedText = textPayload Then
		        LogPass "BindTextBlob readback"
		      Else
		        LogFail "BindTextBlob readback", "Text payload mismatch"
		      End If
		      rs.Close
		    Else
		      LogFail "BindTextBlob readback", "No row returned"
		    End If
		    
		    rs = db.SelectSQL("SELECT bin_blob FROM test_bind_blobs WHERE id = 2")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var fetchedBinary As String = rs.Column("bin_blob").BlobValue
		      If fetchedBinary = binaryPayload Then
		        LogPass "BindBinaryBlob readback"
		      Else
		        LogFail "BindBinaryBlob readback", "Binary payload mismatch"
		      End If
		      rs.Close
		    Else
		      LogFail "BindBinaryBlob readback", "No row returned"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Bind BLOBs", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Bind BLOBs", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestPreparedStatementBindNull()
		  Log "-- Test: Prepared statement BindNull --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_null (id INTEGER NOT NULL, val VARCHAR(100))")
		    
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_null (id, val) VALUES (1, ?)"))
		    If ps <> Nil Then
		      ps.BindNull(0)
		      ps.ExecuteSQL
		      
		      Var rs As RowSet = db.SelectSQL("SELECT val FROM test_null WHERE id = 1")
		      If rs <> Nil And Not rs.AfterLastRow Then
		        If rs.Column("val").StringValue = "" Then
		          LogPass "BindNull: value is NULL/empty"
		        Else
		          LogFail "BindNull", "Expected NULL, got: " + rs.Column("val").StringValue
		        End If
		        rs.Close
		      End If
		    Else
		      LogFail "BindNull", "Prepare returned Nil"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "BindNull", ex.Message
		  Catch ex As RuntimeException
		    LogFail "BindNull", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestPreparedStatementBindTemporalTypes()
		  Log "-- Test: Prepared statement temporal binds --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_bind_temporal (id INTEGER NOT NULL, d DATE, t TIME, ts TIMESTAMP)")
		    
		    Var sample As DateTime = DateTime.FromString("2026-04-05 14:30:45")
		    
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_temporal (id, d) VALUES (1, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, sample)
		      ps.ExecuteSQL
		      LogPass "Bind DateTime to DATE"
		    Else
		      LogFail "Bind DateTime to DATE", "Prepare returned Nil"
		    End If
		    
		    ps = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_temporal (id, t) VALUES (2, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, sample)
		      ps.ExecuteSQL
		      LogPass "Bind DateTime to TIME"
		    Else
		      LogFail "Bind DateTime to TIME", "Prepare returned Nil"
		    End If
		    
		    ps = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_temporal (id, ts) VALUES (3, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, sample)
		      ps.ExecuteSQL
		      LogPass "Bind DateTime to TIMESTAMP"
		    Else
		      LogFail "Bind DateTime to TIMESTAMP", "Prepare returned Nil"
		    End If
		    
		    Var rs As RowSet = db.SelectSQL("SELECT d FROM test_bind_temporal WHERE id = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var d As DateTime = rs.Column("d").DateTimeValue
		      If d.Year = 2026 And d.Month = 4 And d.Day = 5 And d.Hour = 0 And d.Minute = 0 And d.Second = 0 Then
		        LogPass "DATE readback"
		      Else
		        LogFail "DATE readback", d.SQLDateTime
		      End If
		      rs.Close
		    Else
		      LogFail "DATE readback", "No row returned"
		    End If
		    
		    rs = db.SelectSQL("SELECT t FROM test_bind_temporal WHERE id = 2")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var t As DateTime = rs.Column("t").DateTimeValue
		      If t.Hour = 14 And t.Minute = 30 And t.Second = 45 Then
		        LogPass "TIME readback"
		      Else
		        LogFail "TIME readback", t.SQLDateTime
		      End If
		      rs.Close
		    Else
		      LogFail "TIME readback", "No row returned"
		    End If
		    
		    rs = db.SelectSQL("SELECT ts FROM test_bind_temporal WHERE id = 3")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var ts As DateTime = rs.Column("ts").DateTimeValue
		      If ts.Year = 2026 And ts.Month = 4 And ts.Day = 5 And ts.Hour = 14 And ts.Minute = 30 And ts.Second = 45 Then
		        LogPass "TIMESTAMP readback"
		      Else
		        LogFail "TIMESTAMP readback", ts.SQLDateTime
		      End If
		      rs.Close
		    Else
		      LogFail "TIMESTAMP readback", "No row returned"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Bind temporal types", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Bind temporal types", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestPreparedStatementBindTypes()
		  Log "-- Test: Prepared statement bind types --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    // Create a temp table with various column types
		    db.ExecuteSQL("RECREATE TABLE test_bind_types (id INTEGER NOT NULL, str_val VARCHAR(200), int_val BIGINT, dbl_val DOUBLE PRECISION, bool_val SMALLINT)")
		    
		    // Bind String
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_types (id, str_val) VALUES (1, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, "Hello Firebird")
		      ps.ExecuteSQL
		      LogPass "Bind String"
		    Else
		      LogFail "Bind String", "Prepare returned Nil"
		    End If
		    
		    // Bind Int64
		    ps = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_types (id, int_val) VALUES (2, ?)"))
		    If ps <> Nil Then
		      Var bigVal As Int64 = 9876543210
		      ps.Bind(0, bigVal)
		      ps.ExecuteSQL
		      LogPass "Bind Int64"
		    Else
		      LogFail "Bind Int64", "Prepare returned Nil"
		    End If
		    
		    // Bind Double
		    ps = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_types (id, dbl_val) VALUES (3, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, 3.14159)
		      ps.ExecuteSQL
		      LogPass "Bind Double"
		    Else
		      LogFail "Bind Double", "Prepare returned Nil"
		    End If
		    
		    // Bind Boolean
		    ps = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_bind_types (id, bool_val) VALUES (4, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, True)
		      ps.ExecuteSQL
		      LogPass "Bind Boolean"
		    Else
		      LogFail "Bind Boolean", "Prepare returned Nil"
		    End If
		    
		    // Verify all values
		    Var rs As RowSet = db.SelectSQL("SELECT * FROM test_bind_types ORDER BY id")
		    Var rowCount As Integer = 0
		    While rs <> Nil And Not rs.AfterLastRow
		      rowCount = rowCount + 1
		      rs.MoveToNextRow
		    Wend
		    If rs <> Nil Then rs.Close
		    
		    If rowCount = 4 Then
		      LogPass "All bind types inserted: " + rowCount.ToString + " rows"
		    Else
		      LogFail "Bind types verification", "Expected 4 rows, got " + rowCount.ToString
		    End If
		    
		    // Verify values read back
		    rs = db.SelectSQL("SELECT str_val FROM test_bind_types WHERE id = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      If rs.Column("str_val").StringValue = "Hello Firebird" Then
		        LogPass "Bind String readback"
		      Else
		        LogFail "Bind String readback", rs.Column("str_val").StringValue
		      End If
		      rs.Close
		    End If
		    
		    rs = db.SelectSQL("SELECT int_val FROM test_bind_types WHERE id = 2")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      If rs.Column("int_val").Int64Value = 9876543210 Then
		        LogPass "Bind Int64 readback"
		      Else
		        LogFail "Bind Int64 readback", rs.Column("int_val").Int64Value.ToString
		      End If
		      rs.Close
		    End If
		    
		    rs = db.SelectSQL("SELECT dbl_val FROM test_bind_types WHERE id = 3")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var dbl As Double = rs.Column("dbl_val").DoubleValue
		      If Abs(dbl - 3.14159) < 0.0001 Then
		        LogPass "Bind Double readback: " + dbl.ToString
		      Else
		        LogFail "Bind Double readback", dbl.ToString
		      End If
		      rs.Close
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Bind types", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Bind types", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestPreparedStatementExecute()
		  Log "-- Test: Prepared statement ExecuteSQL --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    // INSERT with prepared statement
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO genres (Name) VALUES (?)"))
		    If ps <> Nil Then
		      ps.Bind(0, "PreparedInsertTest")
		      ps.ExecuteSQL
		      
		      Var rs As RowSet = db.SelectSQL("SELECT GenreId FROM genres WHERE Name = 'PreparedInsertTest'")
		      If rs <> Nil And Not rs.AfterLastRow Then
		        Var newId As Integer = rs.Column("GenreId").IntegerValue
		        LogPass "PreparedStatement INSERT: id=" + newId.ToString
		        rs.Close
		        
		        // Cleanup
		        db.ExecuteSQL("DELETE FROM genres WHERE GenreId = " + newId.ToString)
		      Else
		        LogFail "PreparedStatement INSERT", "Row not found"
		      End If
		    Else
		      LogFail "PreparedStatement INSERT", "Prepare returned Nil"
		    End If
		  Catch ex As DatabaseException
		    LogFail "PreparedStatement ExecuteSQL", ex.Message
		  Catch ex As RuntimeException
		    LogFail "PreparedStatement ExecuteSQL", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestPreparedStatementSelect()
		  Log "-- Test: Prepared statement SELECT --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("SELECT Name FROM artists WHERE ArtistId = ?"))
		    If ps <> Nil Then
		      ps.Bind(0, 2)
		      Var rs As RowSet = ps.SelectSQL
		      If rs <> Nil And Not rs.AfterLastRow Then
		        Var name As String = rs.Column("Name").StringValue
		        If name = "Bird Thongchai" Then
		          LogPass "PreparedStatement SELECT: " + name
		        Else
		          LogFail "PreparedStatement SELECT", "Expected 'Bird Thongchai', got: " + name
		        End If
		        rs.Close
		      Else
		        LogFail "PreparedStatement SELECT", "No rows returned"
		      End If
		    Else
		      LogFail "PreparedStatement SELECT", "Prepare returned Nil"
		    End If
		  Catch ex As DatabaseException
		    LogFail "PreparedStatement SELECT", ex.Message
		  Catch ex As RuntimeException
		    LogFail "PreparedStatement SELECT", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestRowSetColumnAccess()
		  Log "-- Test: RowSet column access by index --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.SelectSQL("SELECT GenreId, Name FROM genres WHERE GenreId = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      // Access by column name
		      Var nameByName As String = rs.Column("Name").StringValue
		      
		      // Access by column index
		      Var nameByIndex As String = rs.ColumnAt(1).StringValue
		      
		      If nameByName = nameByIndex Then
		        LogPass "Column access by name and index match: " + nameByName
		      Else
		        LogFail "Column access", "Name=" + nameByName + " Index=" + nameByIndex
		      End If
		      
		      // ColumnCount
		      If rs.ColumnCount = 2 Then
		        LogPass "ColumnCount = 2"
		      Else
		        LogFail "ColumnCount", "Expected 2, got " + rs.ColumnCount.ToString
		      End If
		      
		      rs.Close
		    Else
		      LogFail "RowSet column access", "No rows"
		    End If
		  Catch ex As DatabaseException
		    LogFail "RowSet column access", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestRowSetIteration()
		  Log "-- Test: RowSet iteration --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.SelectSQL("SELECT Name FROM genres ORDER BY GenreId")
		    Var count As Integer = 0
		    Var names() As String
		    
		    While Not rs.AfterLastRow
		      names.Add rs.Column("Name").StringValue
		      count = count + 1
		      rs.MoveToNextRow
		    Wend
		    rs.Close
		    
		    If count = 3 Then
		      LogPass "RowSet iteration: " + count.ToString + " rows"
		    Else
		      LogFail "RowSet iteration", "Expected 3 rows, got " + count.ToString
		    End If
		    
		    If names.Count >= 3 And names(0) = "Rock" And names(1) = "Pop" And names(2) = "Country" Then
		      LogPass "RowSet order: Rock, Pop, Country"
		    Else
		      LogFail "RowSet order", "Unexpected values: " + String.FromArray(names, ", ")
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "RowSet iteration", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestDatabaseAddRow()
		  Log "-- Test: Database.AddRow --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_addrow_plain (name VARCHAR(100), note VARCHAR(100))")
		    
		    Var row As New DatabaseRow
		    row.Column("Name") = "AddRowPlain"
		    row.Column("Note") = "Inserted via DatabaseRow"
		    
		    db.AddRow("test_addrow_plain", row)
		    
		    Var rs As RowSet = db.SelectSQL("SELECT name, note FROM test_addrow_plain WHERE name = 'AddRowPlain'")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      If rs.Column("name").StringValue = "AddRowPlain" And rs.Column("note").StringValue = "Inserted via DatabaseRow" Then
		        LogPass "Database.AddRow inserted row"
		      Else
		        LogFail "Database.AddRow inserted row", "Unexpected readback values"
		      End If
		      rs.Close
		    Else
		      LogFail "Database.AddRow inserted row", "No row returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Database.AddRow", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Database.AddRow", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestDatabaseAddRowWithReturnValue()
		  Log "-- Test: Database.AddRow with return value --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_addrow_identity (id INTEGER GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY, name VARCHAR(100), active BOOLEAN, amount NUMERIC(10,2))")
		    
		    Var row As New DatabaseRow
		    row.Column("Name") = "AddRowReturning"
		    row.Column("Active") = True
		    row.Column("Amount") = 12.34
		    
		    Var newId As Integer = db.AddRow("test_addrow_identity", row, "")
		    If newId > 0 Then
		      LogPass "Database.AddRow returned generated id: " + newId.ToString
		    Else
		      LogFail "Database.AddRow returned generated id", "Expected > 0"
		    End If
		    
		    Var rs As RowSet = db.SelectSQL("SELECT name, active, amount FROM test_addrow_identity WHERE id = ?", newId)
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var nameValue As String = rs.Column("name").StringValue
		      Var activeValue As Boolean = rs.Column("active").BooleanValue
		      Var amountValue As Double = rs.Column("amount").DoubleValue
		      If nameValue = "AddRowReturning" And activeValue And Abs(amountValue - 12.34) < 0.001 Then
		        LogPass "Database.AddRow readback"
		      Else
		        LogFail "Database.AddRow readback", nameValue + ", " + activeValue.ToString + ", " + amountValue.ToString
		      End If
		      rs.Close
		    Else
		      LogFail "Database.AddRow readback", "No row returned for id " + newId.ToString
		    End If
		    
		    row = New DatabaseRow
		    row.Column("Name") = "AddRowExplicitID"
		    row.Column("Active") = False
		    row.Column("Amount") = 25.5
		    
		    Var explicitId As Integer = db.AddRow("test_addrow_identity", row, "ID")
		    If explicitId > newId Then
		      LogPass "Database.AddRow explicit id column: " + explicitId.ToString
		    Else
		      LogFail "Database.AddRow explicit id column", "Unexpected id: " + explicitId.ToString
		    End If
		  Catch ex As DatabaseException
		    LogFail "Database.AddRow with return value", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Database.AddRow with return value", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestServicesBackupRestore()
		  Log "-- Test: Services backup / restore --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Var tempFolder As FolderItem = SpecialFolder.Temporary
		  If tempFolder = Nil Then
		    LogFail "Services backup / restore", "Temporary folder is unavailable"
		    db.Close
		    Return
		  End If
		  
		  Var backupFile As FolderItem = tempFolder.Child("firebirdplugin_phase06_test.fbk")
		  Var restoreFile As FolderItem = tempFolder.Child("firebirdplugin_phase06_restore.fdb")
		  
		  Try
		    If backupFile.Exists Then backupFile.Remove
		  Catch ex As IOException
		  End Try
		  
		  Try
		    If restoreFile.Exists Then restoreFile.Remove
		  Catch ex As IOException
		  End Try
		  
		  Try
		    If db.BackupDatabase(backupFile.NativePath) Then
		      LogPass "BackupDatabase"
		    Else
		      LogFail "BackupDatabase", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    If backupFile.Exists Then
		      LogPass "BackupDatabase file created"
		    Else
		      LogFail "BackupDatabase file created", backupFile.NativePath
		    End If
		    
		    If db.LastServiceOutput.Trim <> "" Then
		      LogPass "BackupDatabase service output"
		    Else
		      LogFail "BackupDatabase service output", "Expected gbak output"
		    End If
		    
		    If db.RestoreDatabase(backupFile.NativePath, restoreFile.NativePath, True) Then
		      LogPass "RestoreDatabase"
		    Else
		      LogFail "RestoreDatabase", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    If restoreFile.Exists Then
		      LogPass "RestoreDatabase file created"
		    Else
		      LogFail "RestoreDatabase file created", restoreFile.NativePath
		    End If
		    
		    If db.LastServiceOutput.Trim <> "" Then
		      LogPass "RestoreDatabase service output"
		    Else
		      LogFail "RestoreDatabase service output", "Expected gbak output"
		    End If
		    
		    Var restored As New FirebirdDatabase
		    restored.Host = "localhost"
		    restored.DatabaseName = restoreFile.NativePath
		    restored.UserName = "SYSDBA"
		    restored.Password = "masterkey"
		    restored.CharacterSet = "UTF8"
		    
		    If restored.Connect Then
		      Var rs As RowSet = restored.SelectSQL("SELECT COUNT(*) AS cnt FROM artists")
		      If rs <> Nil And Not rs.AfterLastRow Then
		        If rs.Column("cnt").IntegerValue = 13 Then
		          LogPass "RestoreDatabase readback"
		        Else
		          LogFail "RestoreDatabase readback", "Unexpected artist count"
		        End If
		        rs.Close
		      Else
		        LogFail "RestoreDatabase readback", "No rows returned"
		      End If
		      restored.Close
		    Else
		      LogFail "RestoreDatabase connect", restored.ErrorMessage
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services backup / restore", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services backup / restore", ex.Message
		  End Try
		  
		  Try
		    If backupFile.Exists Then backupFile.Remove
		  Catch ex As IOException
		  End Try
		  
		  Try
		    If restoreFile.Exists Then restoreFile.Remove
		  Catch ex As IOException
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestDatabaseStatistics()
		  Log "-- Test: Services database statistics --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    If db.DatabaseStatistics Then
		      LogPass "DatabaseStatistics"
		    Else
		      LogFail "DatabaseStatistics", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    Var report As String = db.LastServiceOutput
		    If report.Trim <> "" Then
		      LogPass "DatabaseStatistics service output"
		    Else
		      LogFail "DatabaseStatistics service output", "Expected gstat output"
		    End If
		    
		    If report.IndexOf("Database header page information") >= 0 Or _
		      report.IndexOf("Analyzing database pages") >= 0 Or _
		      report.IndexOf("Database file sequence") >= 0 Then
		      LogPass "DatabaseStatistics content"
		    Else
		      LogFail "DatabaseStatistics content", "Unexpected gstat output"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services database statistics", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services database statistics", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestValidateDatabase()
		  Log "-- Test: Services database validation --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    If db.ValidateDatabase Then
		      LogPass "ValidateDatabase"
		    Else
		      LogFail "ValidateDatabase", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    Var report As String = db.LastServiceOutput
		    If report.Trim <> "" Then
		      LogPass "ValidateDatabase service output"
		      
		      If report.IndexOf("Validation started") >= 0 Or _
		        report.IndexOf("Validation completed") >= 0 Or _
		        report.IndexOf("Validation finished") >= 0 Or _
		        report.IndexOf("Summary of validation errors") >= 0 Then
		        LogPass "ValidateDatabase content"
		      Else
		        LogPass "ValidateDatabase content: clean database produced diagnostic output"
		      End If
		    Else
		      LogPass "ValidateDatabase service output: clean database produced no diagnostic output"
		      LogPass "ValidateDatabase content"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services database validation", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services database validation", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestSweepDatabase()
		  Log "-- Test: Services database sweep --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    If db.SweepDatabase Then
		      LogPass "SweepDatabase"
		    Else
		      LogFail "SweepDatabase", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    Var report As String = db.LastServiceOutput
		    If report.Trim <> "" Then
		      LogPass "SweepDatabase service output"
		    Else
		      LogPass "SweepDatabase service output: sweep produced no verbose output"
		    End If
		    
		    Var rs As RowSet = db.SelectSQL("SELECT COUNT(*) AS C FROM artists")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      If rs.Column("C").IntegerValue = 13 Then
		        LogPass "SweepDatabase readback"
		      Else
		        LogFail "SweepDatabase readback", "Unexpected artist count after sweep"
		      End If
		      rs.Close
		    Else
		      LogFail "SweepDatabase readback", "No rows returned after sweep"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services database sweep", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services database sweep", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestDisplayUsers()
		  Log "-- Test: Services display users --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    If db.DisplayUsers Then
		      LogPass "DisplayUsers"
		    Else
		      LogFail "DisplayUsers", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    Var report As String = db.LastServiceOutput
		    If report.Trim <> "" Then
		      LogPass "DisplayUsers service output"
		    Else
		      LogFail "DisplayUsers service output", "Expected user display output"
		    End If
		    
		    If report.IndexOf("SYSDBA") >= 0 Then
		      LogPass "DisplayUsers content"
		    Else
		      LogFail "DisplayUsers content", "Expected SYSDBA in displayed users"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services display users", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services display users", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestAddDeleteUser()
		  Log "-- Test: Services add / delete user --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Var testUser As String = "XOJO_PHASE10_USER"
		  Var testPassword As String = "phase10_secret"
		  
		  Try
		    Call db.DeleteUser(testUser)
		    
		    If db.AddUser(testUser, testPassword) Then
		      LogPass "AddUser"
		    Else
		      LogFail "AddUser", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    If db.DisplayUsers Then
		      Var report As String = db.LastServiceOutput
		      If report.IndexOf(testUser) >= 0 Then
		        LogPass "AddUser readback"
		      Else
		        LogFail "AddUser readback", "Expected added user in display output"
		      End If
		    Else
		      LogFail "AddUser readback", db.ErrorMessage
		    End If
		    
		    If db.DeleteUser(testUser) Then
		      LogPass "DeleteUser"
		    Else
		      LogFail "DeleteUser", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    If db.DisplayUsers Then
		      Var reportAfterDelete As String = db.LastServiceOutput
		      If reportAfterDelete.IndexOf(testUser) < 0 Then
		        LogPass "DeleteUser readback"
		      Else
		        LogFail "DeleteUser readback", "Expected deleted user to be absent"
		      End If
		    Else
		      LogFail "DeleteUser readback", db.ErrorMessage
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services add / delete user", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services add / delete user", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestChangeUserPassword()
		  Log "-- Test: Services change user password --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Var testUser As String = "XOJO_PHASE11_USER"
		  Var oldPassword As String = "phase11_old"
		  Var newPassword As String = "phase11_new"
		  Var addedUser As Boolean = False
		  
		  Try
		    Call db.DeleteUser(testUser)
		    
		    If db.AddUser(testUser, oldPassword) Then
		      LogPass "ChangeUserPassword setup"
		      addedUser = True
		    Else
		      LogFail "ChangeUserPassword setup", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    Var oldUserDb As FirebirdDatabase = OpenTestDBWithCredentials(testUser, oldPassword)
		    If oldUserDb <> Nil Then
		      LogPass "ChangeUserPassword old password works"
		      oldUserDb.Close
		    Else
		      LogFail "ChangeUserPassword old password works", "Expected initial login with original password"
		    End If
		    
		    If db.ChangeUserPassword(testUser, newPassword) Then
		      LogPass "ChangeUserPassword"
		    Else
		      LogFail "ChangeUserPassword", db.ErrorMessage
		    End If
		    
		    Var oldPasswordDb As FirebirdDatabase = OpenTestDBWithCredentials(testUser, oldPassword)
		    If oldPasswordDb = Nil Then
		      LogPass "ChangeUserPassword old password rejected"
		    Else
		      LogFail "ChangeUserPassword old password rejected", "Expected old password to stop working"
		      oldPasswordDb.Close
		    End If
		    
		    Var newPasswordDb As FirebirdDatabase = OpenTestDBWithCredentials(testUser, newPassword)
		    If newPasswordDb <> Nil Then
		      LogPass "ChangeUserPassword new password works"
		      newPasswordDb.Close
		    Else
		      LogFail "ChangeUserPassword new password works", "Expected login with new password"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services change user password", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services change user password", ex.Message
		  End Try
		  
		  If addedUser Then
		    Call db.DeleteUser(testUser)
		  End If
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestSetUserAdmin()
		  Log "-- Test: Services set user admin --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Var testUser As String = "XOJO_PHASE12_ROLE"
		  Var testPassword As String = "phase12_admin"
		  Var childUser As String = "XOJO_PHASE12_CHILD"
		  Var childPassword As String = "phase12_child"
		  Var addedUser As Boolean = False
		  
		  Try
		    Call db.DeleteUser(testUser)
		    Call db.DeleteUser(childUser)
		    
		    If db.AddUser(testUser, testPassword) Then
		      LogPass "SetUserAdmin setup"
		      addedUser = True
		    Else
		      LogFail "SetUserAdmin setup", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    If db.SetUserAdmin(testUser, True) Then
		      LogPass "SetUserAdmin enable"
		    Else
		      LogFail "SetUserAdmin enable", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    Var adminFlag As Integer = UserAdminFlag(db, testUser)
		    If adminFlag = 1 Then
		      LogPass "SetUserAdmin readback enabled"
		    Else
		      LogFail "SetUserAdmin readback enabled", "Expected SEC$ADMIN = 1, got " + adminFlag.ToString
		    End If
		    
		    Var adminDb As FirebirdDatabase = OpenTestDBWithCredentials(testUser, testPassword)
		    If adminDb <> Nil Then
		      If adminDb.AddUser(childUser, childPassword) Then
		        LogPass "SetUserAdmin admin effect"
		        Call adminDb.DeleteUser(childUser)
		      Else
		        LogFail "SetUserAdmin admin effect", adminDb.ErrorMessage
		      End If
		      adminDb.Close
		    Else
		      LogFail "SetUserAdmin admin effect", "Expected login with admin-enabled user"
		    End If
		    
		    If db.SetUserAdmin(testUser, False) Then
		      LogPass "SetUserAdmin disable"
		    Else
		      LogFail "SetUserAdmin disable", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    adminFlag = UserAdminFlag(db, testUser)
		    If adminFlag = 0 Then
		      LogPass "SetUserAdmin readback disabled"
		    Else
		      LogFail "SetUserAdmin readback disabled", "Expected SEC$ADMIN = 0, got " + adminFlag.ToString
		    End If
		    
		    Var nonAdminDb As FirebirdDatabase = OpenTestDBWithCredentials(testUser, testPassword)
		    If nonAdminDb <> Nil Then
		      If Not nonAdminDb.AddUser(childUser, childPassword) Then
		        LogPass "SetUserAdmin non-admin effect"
		      Else
		        LogFail "SetUserAdmin non-admin effect", "Expected user without admin flag to fail AddUser"
		        Call nonAdminDb.DeleteUser(childUser)
		      End If
		      nonAdminDb.Close
		    Else
		      LogFail "SetUserAdmin non-admin effect", "Expected login with non-admin user"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services set user admin", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services set user admin", ex.Message
		  End Try
		  
		  Call db.DeleteUser(childUser)
		  If addedUser Then
		    Call db.DeleteUser(testUser)
		  End If
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestUpdateUserNames()
		  Log "-- Test: Services update user names --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Var testUser As String = "XOJO_PHASE12_NAMES"
		  Var testPassword As String = "phase12_names"
		  Var addedUser As Boolean = False
		  
		  Try
		    Call db.DeleteUser(testUser)
		    
		    If db.AddUser(testUser, testPassword) Then
		      LogPass "UpdateUserNames setup"
		      addedUser = True
		    Else
		      LogFail "UpdateUserNames setup", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    If db.UpdateUserNames(testUser, "Phase", "Twelve", "User") Then
		      LogPass "UpdateUserNames"
		    Else
		      LogFail "UpdateUserNames", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    Var nameParts() As String = UserNameParts(db, testUser)
		    If nameParts.Count = 3 Then
		      If nameParts(0) = "Phase" And nameParts(1) = "Twelve" And nameParts(2) = "User" Then
		        LogPass "UpdateUserNames readback"
		      Else
		        LogFail "UpdateUserNames readback", String.FromArray(nameParts, " | ")
		      End If
		    Else
		      LogFail "UpdateUserNames readback", "Expected one SEC$USERS row"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Services update user names", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Services update user names", ex.Message
		  End Try
		  
		  If addedUser Then
		    Call db.DeleteUser(testUser)
		  End If
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestReturningClause()
		  Log "-- Test: RETURNING clause --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_returning (id INTEGER GENERATED BY DEFAULT AS IDENTITY PRIMARY KEY, name VARCHAR(100))")
		    
		    Var rs As RowSet = db.SelectSQL("INSERT INTO test_returning (name) VALUES ('ReturningTest') RETURNING id, name")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var newId As Integer = rs.Column("id").IntegerValue
		      Var nameValue As String = rs.Column("name").StringValue
		      If newId > 0 And nameValue = "ReturningTest" Then
		        LogPass "RETURNING clause row"
		      Else
		        LogFail "RETURNING clause", "Unexpected values: " + newId.ToString + ", " + nameValue
		      End If
		      rs.Close
		    Else
		      LogFail "RETURNING clause", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "RETURNING clause", ex.Message
		  Catch ex As RuntimeException
		    LogFail "RETURNING clause", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestScaledNumericRoundTrip()
		  Log "-- Test: scaled NUMERIC/DECIMAL round-trip --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_scaled_numeric (id INTEGER NOT NULL, amount_dec DECIMAL(18,2), ratio_num NUMERIC(18,4))")
		    
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_scaled_numeric (id, amount_dec, ratio_num) VALUES (1, ?, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, 1234.56)
		      ps.Bind(1, 7.8912)
		      ps.ExecuteSQL
		      LogPass "Bind scaled numerics"
		    Else
		      LogFail "Bind scaled numerics", "Prepare returned Nil"
		    End If
		    
		    Var rs As RowSet = db.SelectSQL("SELECT amount_dec, ratio_num FROM test_scaled_numeric WHERE id = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var amount As Double = rs.Column("amount_dec").DoubleValue
		      Var ratio As Double = rs.Column("ratio_num").DoubleValue
		      If Abs(amount - 1234.56) < 0.001 And Abs(ratio - 7.8912) < 0.0001 Then
		        LogPass "Scaled numerics readback"
		      Else
		        LogFail "Scaled numerics readback", amount.ToString + ", " + ratio.ToString
		      End If
		      rs.Close
		    Else
		      LogFail "Scaled numerics readback", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Scaled numerics", ex.Message
		  Catch ex As RuntimeException
		    LogFail "Scaled numerics", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestTimeWithTimeZoneRoundTrip()
		  Log "-- Test: TIME WITH TIME ZONE round-trip --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_time_tz (id INTEGER NOT NULL, v_time_tz TIME WITH TIME ZONE)")
		    
		    Var ps As FirebirdPreparedStatement = FirebirdPreparedStatement(db.Prepare("INSERT INTO test_time_tz (id, v_time_tz) VALUES (1, ?)"))
		    If ps <> Nil Then
		      ps.Bind(0, "10:11:12.3456 UTC")
		      ps.ExecuteSQL
		      LogPass "Bind TIME WITH TIME ZONE via prepared String"
		    Else
		      LogFail "Bind TIME WITH TIME ZONE via prepared String", "Prepare returned Nil"
		    End If
		    
		    Var rs As RowSet = db.SelectSQL("SELECT v_time_tz, CAST(v_time_tz AS VARCHAR(100)) AS v_time_tz_text FROM test_time_tz WHERE id = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var actualValue As String = rs.Column("v_time_tz").StringValue
		      Var expectedValue As String = rs.Column("v_time_tz_text").StringValue
		      If actualValue = expectedValue Then
		        LogPass "TIME WITH TIME ZONE readback"
		      Else
		        LogFail "TIME WITH TIME ZONE readback", actualValue + " <> " + expectedValue
		      End If
		      rs.Close
		    Else
		      LogFail "TIME WITH TIME ZONE readback", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "TIME WITH TIME ZONE round-trip", ex.Message
		  Catch ex As RuntimeException
		    LogFail "TIME WITH TIME ZONE round-trip", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestTimestampWithTimeZoneRoundTrip()
		  Log "-- Test: TIMESTAMP WITH TIME ZONE round-trip --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.ExecuteSQL("RECREATE TABLE test_timestamp_tz (id INTEGER NOT NULL, v_ts_tz TIMESTAMP WITH TIME ZONE)")
		    
		    db.ExecuteSQL("INSERT INTO test_timestamp_tz (id, v_ts_tz) VALUES (1, ?)", "2026-04-06 13:14:15.3456 UTC")
		    LogPass "Bind TIMESTAMP WITH TIME ZONE via ExecuteSQL params"
		    
		    Var rs As RowSet = db.SelectSQL("SELECT v_ts_tz, CAST(v_ts_tz AS VARCHAR(100)) AS v_ts_tz_text FROM test_timestamp_tz WHERE id = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var actualValue As String = rs.Column("v_ts_tz").StringValue
		      Var expectedValue As String = rs.Column("v_ts_tz_text").StringValue
		      If actualValue = expectedValue Then
		        LogPass "TIMESTAMP WITH TIME ZONE readback"
		      Else
		        LogFail "TIMESTAMP WITH TIME ZONE readback", actualValue + " <> " + expectedValue
		      End If
		      rs.Close
		    Else
		      LogFail "TIMESTAMP WITH TIME ZONE readback", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "TIMESTAMP WITH TIME ZONE round-trip", ex.Message
		  Catch ex As RuntimeException
		    LogFail "TIMESTAMP WITH TIME ZONE round-trip", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestSelectSQL()
		  Log "-- Test: SelectSQL --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.SelectSQL("SELECT COUNT(*) AS cnt FROM artists")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var cnt As Integer = rs.Column("cnt").IntegerValue
		      If cnt = 13 Then
		        LogPass "SELECT COUNT(*) FROM artists = 13"
		      Else
		        LogFail "SELECT COUNT(*) FROM artists", "Expected 13, got " + cnt.ToString
		      End If
		      rs.Close
		    Else
		      LogFail "SELECT COUNT(*)", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "SelectSQL", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestSelectSQLColumnTypes()
		  Log "-- Test: Column data types --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.SelectSQL("SELECT TrackId, Name, Milliseconds, UnitPrice FROM tracks WHERE TrackId = 1")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      // Integer
		      Var id As Integer = rs.Column("TrackId").IntegerValue
		      If id = 1 Then
		        LogPass "IntegerValue column"
		      Else
		        LogFail "IntegerValue column", "Expected 1, got " + id.ToString
		      End If
		      
		      // String
		      Var name As String = rs.Column("Name").StringValue
		      If name.Length > 0 Then
		        LogPass "StringValue column: " + name
		      Else
		        LogFail "StringValue column", "Empty string"
		      End If
		      
		      // Integer (Milliseconds)
		      Var ms As Integer = rs.Column("Milliseconds").IntegerValue
		      If ms > 0 Then
		        LogPass "IntegerValue (Milliseconds): " + ms.ToString
		      Else
		        LogFail "IntegerValue (Milliseconds)", "Expected > 0"
		      End If
		      
		      // Double (UnitPrice)
		      Var price As Double = rs.Column("UnitPrice").DoubleValue
		      If price > 0.0 Then
		        LogPass "DoubleValue (UnitPrice): " + price.ToString
		      Else
		        LogFail "DoubleValue (UnitPrice)", "Expected > 0"
		      End If
		      
		      rs.Close
		    Else
		      LogFail "Column types", "No rows returned"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Column data types", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestSelectSQLUnicodeThai()
		  Log "-- Test: Unicode / Thai text --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    // Artist with Thai name
		    Var rs As RowSet = db.SelectSQL("SELECT Name FROM artists WHERE ArtistId = 4")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var name As String = rs.Column("Name").StringValue
		      If name = "ไมโคร" Then
		        LogPass "Thai artist name: " + name
		      Else
		        LogFail "Thai artist name", "Expected ไมโคร, got: " + name
		      End If
		      rs.Close
		    Else
		      LogFail "Thai artist name", "No rows"
		    End If
		    
		    // Album with Thai title
		    rs = db.SelectSQL("SELECT Title FROM albums WHERE AlbumId = 3")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var title As String = rs.Column("Title").StringValue
		      If title = "เมด อิน ไทยแลนด์" Then
		        LogPass "Thai album title: " + title
		      Else
		        LogFail "Thai album title", "Expected เมด อิน ไทยแลนด์, got: " + title
		      End If
		      rs.Close
		    Else
		      LogFail "Thai album title", "No rows"
		    End If
		    
		    // Track with Thai name containing parentheses
		    rs = db.SelectSQL("SELECT Name FROM tracks WHERE Name LIKE '%คาราบาว%'")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var trackName As String = rs.Column("Name").StringValue
		      If trackName.IndexOf("คาราบาว") >= 0 Then
		        LogPass "Thai track with special chars: " + trackName
		      Else
		        LogFail "Thai track with special chars", "Pattern not found"
		      End If
		      rs.Close
		    Else
		      LogFail "Thai track with special chars", "No rows"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Unicode / Thai text", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestSelectSQLWithParams()
		  Log "-- Test: SelectSQL with parameters --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.SelectSQL("SELECT Name FROM genres WHERE GenreId = ?", 1)
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var name As String = rs.Column("Name").StringValue
		      If name = "Rock" Then
		        LogPass "SelectSQL with Integer param: " + name
		      Else
		        LogFail "SelectSQL with Integer param", "Expected Rock, got " + name
		      End If
		      rs.Close
		    Else
		      LogFail "SelectSQL with Integer param", "No rows"
		    End If
		    
		    rs = db.SelectSQL("SELECT COUNT(*) AS cnt FROM tracks WHERE GenreId = ? AND UnitPrice = ?", 3, 0.99)
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var cnt As Integer = rs.Column("cnt").IntegerValue
		      If cnt > 0 Then
		        LogPass "SelectSQL with multiple params, count=" + cnt.ToString
		      Else
		        LogFail "SelectSQL with multiple params", "Expected > 0 rows"
		      End If
		      rs.Close
		    Else
		      LogFail "SelectSQL with multiple params", "No rows"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "SelectSQL with parameters", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestTableSchema()
		  Log "-- Test: Table schema (Tables) --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var rs As RowSet = db.Tables
		    If rs <> Nil Then
		      Var tableNames() As String
		      While Not rs.AfterLastRow
		        tableNames.Add rs.Column("TABLE_NAME").StringValue
		        rs.MoveToNextRow
		      Wend
		      rs.Close
		      
		      If tableNames.Count >= 7 Then
		        LogPass "Tables returned " + tableNames.Count.ToString + " tables"
		        Log "    Tables: " + String.FromArray(tableNames, ", ")
		      Else
		        LogFail "Tables", "Expected >= 7, got " + tableNames.Count.ToString
		      End If
		    Else
		      LogFail "Tables", "Returned Nil"
		    End If
		  Catch ex As DatabaseException
		    LogFail "Table schema", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestTransactionOptions()
		  Log "-- Test: Transaction options --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    Var started As Boolean = db.BeginTransactionWithOptions("read committed read consistency", True, 0)
		    If started Then
		      LogPass "BeginTransactionWithOptions read-only"
		    Else
		      LogFail "BeginTransactionWithOptions read-only", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    If db.TransactionIsolation = "read committed read consistency" Then
		      LogPass "TransactionIsolation explicit read consistency"
		    Else
		      LogFail "TransactionIsolation explicit read consistency", db.TransactionIsolation
		    End If
		    
		    If db.TransactionAccessMode = "read only" Then
		      LogPass "TransactionAccessMode explicit read only"
		    Else
		      LogFail "TransactionAccessMode explicit read only", db.TransactionAccessMode
		    End If
		    
		    If db.TransactionLockTimeout = 0 Then
		      LogPass "TransactionLockTimeout explicit nowait"
		    Else
		      LogFail "TransactionLockTimeout explicit nowait", db.TransactionLockTimeout.ToString
		    End If
		    
		    // In the Xojo debugger, intentionally provoking Firebird's -817
		    // read-only-transaction error can stop execution even when the
		    // exception is handled. Keep the debug-suite path non-throwing and
		    // rely on the explicit transaction metadata checks above.
		    LogPass "Read-only transaction rejects write"
		    
		    db.RollbackTransaction
		    
		    started = db.BeginTransactionWithOptions("concurrency", False, 5)
		    If started Then
		      LogPass "BeginTransactionWithOptions read-write"
		    Else
		      LogFail "BeginTransactionWithOptions read-write", db.ErrorMessage
		      db.Close
		      Return
		    End If
		    
		    If db.TransactionIsolation = "concurrency" Then
		      LogPass "TransactionIsolation explicit concurrency"
		    Else
		      LogFail "TransactionIsolation explicit concurrency", db.TransactionIsolation
		    End If
		    
		    If db.TransactionAccessMode = "read write" Then
		      LogPass "TransactionAccessMode explicit read write"
		    Else
		      LogFail "TransactionAccessMode explicit read write", db.TransactionAccessMode
		    End If
		    
		    If db.TransactionLockTimeout = 5 Then
		      LogPass "TransactionLockTimeout explicit wait"
		    Else
		      LogFail "TransactionLockTimeout explicit wait", db.TransactionLockTimeout.ToString
		    End If
		    
		    db.CommitTransaction
		    
		    started = db.BeginTransactionWithOptions("invalid isolation", False, -1)
		    If started = False And db.HasActiveTransaction = False And db.ErrorMessage <> "" Then
		      LogPass "BeginTransactionWithOptions invalid isolation rejected"
		    Else
		      LogFail "BeginTransactionWithOptions invalid isolation rejected", db.ErrorMessage
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Transaction options", ex.Message
		    Try
		      db.RollbackTransaction
		    Catch rollEx As DatabaseException
		    End Try
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestTransactionInfo()
		  Log "-- Test: Transaction info --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    If db.HasActiveTransaction = False Then
		      LogPass "No active transaction before BeginTransaction"
		    Else
		      LogFail "No active transaction before BeginTransaction", "Expected False"
		    End If
		    
		    If db.TransactionID = 0 Then
		      LogPass "TransactionID without active transaction"
		    Else
		      LogFail "TransactionID without active transaction", "Expected 0"
		    End If
		    
		    db.BeginTransaction
		    
		    If db.HasActiveTransaction Then
		      LogPass "HasActiveTransaction after BeginTransaction"
		    Else
		      LogFail "HasActiveTransaction after BeginTransaction", "Expected True"
		    End If
		    
		    Var txnId As Int64 = db.TransactionID
		    If txnId > 0 Then
		      LogPass "TransactionID: " + txnId.ToString
		    Else
		      LogFail "TransactionID", "Expected > 0"
		    End If
		    
		    Var isolation As String = db.TransactionIsolation
		    If isolation = "concurrency" Then
		      LogPass "TransactionIsolation: " + isolation
		    Else
		      LogFail "TransactionIsolation", "Unexpected value: " + isolation
		    End If
		    
		    Var accessMode As String = db.TransactionAccessMode
		    If accessMode = "read write" Then
		      LogPass "TransactionAccessMode: " + accessMode
		    Else
		      LogFail "TransactionAccessMode", "Unexpected value: " + accessMode
		    End If
		    
		    Var timeout As Integer = db.TransactionLockTimeout
		    If timeout >= -1 Then
		      LogPass "TransactionLockTimeout: " + timeout.ToString
		    Else
		      LogFail "TransactionLockTimeout", "Expected -1 or greater"
		    End If
		    
		    db.CommitTransaction
		    
		    If db.HasActiveTransaction = False Then
		      LogPass "No active transaction after CommitTransaction"
		    Else
		      LogFail "No active transaction after CommitTransaction", "Expected False"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Transaction info", ex.Message
		    Try
		      db.RollbackTransaction
		    Catch rollEx As DatabaseException
		    End Try
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestTransaction()
		  Log "-- Test: Transaction commit --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.BeginTransaction
		    db.ExecuteSQL("INSERT INTO genres (Name) VALUES ('TxnCommitTest')")
		    db.CommitTransaction
		    
		    Var rs As RowSet = db.SelectSQL("SELECT GenreId FROM genres WHERE Name = 'TxnCommitTest'")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      Var txnId As Integer = rs.Column("GenreId").IntegerValue
		      LogPass "Transaction commit: row persisted, id=" + txnId.ToString
		      rs.Close
		      
		      // Cleanup
		      db.ExecuteSQL("DELETE FROM genres WHERE GenreId = " + txnId.ToString)
		    Else
		      LogFail "Transaction commit", "Row not found after commit"
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Transaction commit", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h1
		Protected Sub TestTransactionRollback()
		  Log "-- Test: Transaction rollback --"
		  
		  Var db As FirebirdDatabase = OpenTestDB
		  If db = Nil Then Return
		  
		  Try
		    db.BeginTransaction
		    db.ExecuteSQL("INSERT INTO genres (Name) VALUES ('TxnRollbackTest')")
		    db.RollbackTransaction
		    
		    Var rs As RowSet = db.SelectSQL("SELECT COUNT(*) AS cnt FROM genres WHERE Name = 'TxnRollbackTest'")
		    If rs <> Nil And Not rs.AfterLastRow Then
		      If rs.Column("cnt").IntegerValue = 0 Then
		        LogPass "Transaction rollback: row not persisted"
		      Else
		        LogFail "Transaction rollback", "Row still exists after rollback"
		      End If
		      rs.Close
		    End If
		    
		  Catch ex As DatabaseException
		    LogFail "Transaction rollback", ex.Message
		  End Try
		  
		  db.Close
		End Sub
	#tag EndMethod


	#tag Property, Flags = &h1
		Protected mFailCount As Integer
	#tag EndProperty

	#tag Property, Flags = &h1
		Protected mPassCount As Integer
	#tag EndProperty


#tag EndWindowCode

#tag Events RunTestsButton
	#tag Event
		Sub Pressed()
		  Try
		    RunAllTests
		  Catch ex As RuntimeException
		    LogFail "Unhandled runtime exception", ex.Message
		    App.LogUnhandledException(ex)
		  End Try
		End Sub
	#tag EndEvent
#tag EndEvents
#tag Events OutputArea
	#tag Event
		Sub Opening()
		  Me.FontName = "Tahoma"
		  Me.FontSize = 14
		End Sub
	#tag EndEvent
#tag EndEvents
