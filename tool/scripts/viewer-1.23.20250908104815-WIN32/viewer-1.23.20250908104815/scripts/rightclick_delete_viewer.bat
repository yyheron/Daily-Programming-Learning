
set "script_dir=%~dp0"
@REM reg delete HKCR\*\shell\viewer /f
@REM reg delete HKCR\Directory\shell\viewer_opendir /f
@REM reg delete HKCR\Directory\shell\viewer_seek /f
@REM reg delete HKCR\Directory\shell\viewer_seek2 /f
@REM reg delete HKCR\Directory\Background\shell\viewer_opendir /f
@REM reg delete HKCR\Directory\Background\shell\viewer_seek /f
@REM reg delete HKCR\Directory\Background\shell\viewer_seek2 /f

reg delete HKCR\*\shell\viewer /f
reg delete HKCR\Directory\shell\viewer0 /f
reg delete HKCR\Directory\shell\viewer1 /f
reg delete HKCR\Directory\Background\shell\viewer0 /f
reg delete HKCR\Directory\Background\shell\viewer1 /f
reg delete HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek /f
reg delete HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek0 /f
reg delete HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek1 /f
reg delete HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek2 /f
reg delete HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek /f
reg delete HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek0 /f
reg delete HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek1 /f
reg delete HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek2 /f