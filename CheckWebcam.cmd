@Echo ON
:: CheckWebcam
%~dp0/CheckWebcam.exe
If "%ERRORLEVEL%"=="0" (Goto :CREATE) Else (Goto :RESTORE)
:: END CALLOUT B
Goto :END

:RESTORE
powershell.exe -NoExit -ExecutionPolicy ByPass -File %~dp0/Elevate.ps1 %~dp0/RestoreLatest.ps1"
Goto :END

:CREATE
powershell.exe -ExecutionPolicy ByPass -File %~dp0/Elevate.ps1 -ExecScript %~dp0/CreateRestore.ps1
:END

Pause