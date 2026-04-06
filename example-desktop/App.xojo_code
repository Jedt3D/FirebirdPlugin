#tag Class
Protected Class App
Inherits DesktopApplication
	#tag Event
		Function UnhandledException(error As RuntimeException) As Boolean
		  LogUnhandledException(error)
		  Return True
		End Function
	#tag EndEvent

	#tag Constant, Name = kEditClear, Type = String, Dynamic = False, Default = \"&Delete", Scope = Public
		#Tag Instance, Platform = Windows, Language = Default, Definition  = \"&Delete"
		#Tag Instance, Platform = Linux, Language = Default, Definition  = \"&Delete"
	#tag EndConstant

	#tag Constant, Name = kFileQuit, Type = String, Dynamic = False, Default = \"&Quit", Scope = Public
		#Tag Instance, Platform = Windows, Language = Default, Definition  = \"E&xit"
	#tag EndConstant

	#tag Constant, Name = kFileQuitShortcut, Type = String, Dynamic = False, Default = \"", Scope = Public
		#Tag Instance, Platform = Mac OS, Language = Default, Definition  = \"Cmd+Q"
		#Tag Instance, Platform = Linux, Language = Default, Definition  = \"Ctrl+Q"
	#tag EndConstant

	#tag Method, Flags = &h0
		Sub LogUnhandledException(error As RuntimeException)
		  Var details As String = "Unhandled exception at " + DateTime.Now.SQLDateTime
		  
		  If error <> Nil Then
		    details = details + EndOfLine + "Message: " + error.Message
		  Else
		    details = details + EndOfLine + "Message: <nil RuntimeException>"
		  End If
		  
		  System.DebugLog(details)
		  
		  Try
		    Var logFile As FolderItem = SpecialFolder.Temporary.Child("firebirdplugin-tests-unhandled-" + DateTime.Now.SecondsFrom1970.ToString + ".log")
		    Var output As TextOutputStream = TextOutputStream.Create(logFile)
		    output.Write(details + EndOfLine)
		    output.Close
		  Catch ioErr As IOException
		    System.DebugLog("Failed to write unhandled exception log: " + ioErr.Message)
		  End Try
		End Sub
	#tag EndMethod

End Class
#tag EndClass
