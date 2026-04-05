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
		Protected Function OpenTestDB() As FirebirdDatabase
		  Var db As New FirebirdDatabase
		  db.Host = "localhost"
		  db.DatabaseName = "/Users/worajedt/Xojo Projects/FirebirdPlugin/music.fdb"
		  db.UserName = "SYSDBA"
		  db.Password = "masterkey"
		  db.CharacterSet = "UTF8"
		  
		  If Not db.Connect Then
		    Log "  ERROR: Could not connect -- " + db.ErrorMessage
		    Return Nil
		  End If
		  
		  Return db
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
		  TestSelectSQL
		  TestSelectSQLColumnTypes
		  TestSelectSQLUnicodeThai
		  TestSelectSQLWithParams
		  TestRowSetIteration
		  TestRowSetColumnAccess
		  TestExecuteSQL
		  TestTransaction
		  TestTransactionRollback
		  TestPreparedStatementSelect
		  TestPreparedStatementExecute
		  TestPreparedStatementBindTypes
		  TestPreparedStatementBindNull
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
		  RunAllTests
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
