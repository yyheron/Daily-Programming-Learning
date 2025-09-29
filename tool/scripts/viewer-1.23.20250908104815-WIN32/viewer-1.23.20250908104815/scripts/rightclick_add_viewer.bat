set "script_dir=%~dp0"
regedit /s %script_dir%\helpers\viewer.reg

reg add HKCR\*\shell\viewer /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKCR\*\shell\viewer /v MuiVerb /t REG_SZ /d "Open with Viewer(&`)" /f
reg add HKCR\*\shell\viewer\command /d "%script_dir%\open_file.bat %%1" /f

reg add HKCR\Directory\shell\viewer0 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKCR\Directory\shell\viewer0 /v MuiVerb /t REG_SZ /d "Search with Viewer(&!)" /f
reg add HKCR\Directory\shell\viewer0 /v SubCommands /t REG_SZ /d "viewer.nseek;viewer.nseek0;viewer.nseek1;viewer.nseek2" /f
reg add HKCR\Directory\shell\viewer1 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKCR\Directory\shell\viewer1 /v MuiVerb /t REG_SZ /d "Seawch and group with Viewer(&`)" /f
reg add HKCR\Directory\shell\viewer1 /v SubCommands /t REG_SZ /d "viewer.seek;viewer.seek0;viewer.seek1;viewer.seek2" /f
reg add HKCR\Directory\Background\shell\viewer0 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKCR\Directory\Background\shell\viewer0 /v MuiVerb /t REG_SZ /d "Search with Viewer(&!)" /f
reg add HKCR\Directory\Background\shell\viewer0 /v SubCommands /t REG_SZ /d "viewer.nseek;viewer.nseek0;viewer.nseek1;viewer.nseek2" /f
reg add HKCR\Directory\Background\shell\viewer1 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKCR\Directory\Background\shell\viewer1 /v MuiVerb /t REG_SZ /d "Search and group with Viewer(&`)" /f
reg add HKCR\Directory\Background\shell\viewer1 /v SubCommands /t REG_SZ /d "viewer.seek;viewer.seek0;viewer.seek1;viewer.seek2" /f

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek /v MuiVerb /t REG_SZ /d "Search only(&`)" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek\command /t REG_EXPAND_SZ /d "%script_dir%\seek_and_group.bat %%V -" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek0 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek0 /v MuiVerb /t REG_SZ /d "Specify calibs dir" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek0\command /t REG_EXPAND_SZ /d "%script_dir%\seek_and_group.bat %%V" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek1 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek1 /v MuiVerb /t REG_SZ /d "Search calibs here" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek1\command /t REG_EXPAND_SZ /d "%script_dir%\seek_and_group.bat %%V %%V" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek2 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek2 /v MuiVerb /t REG_SZ /d "Search calibs at up level dir(&!)" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.seek2\command /t REG_EXPAND_SZ /d "%script_dir%\seek_and_group.bat %%V %%V\.." /f

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek /v MuiVerb /t REG_SZ /d "Search only(&`)" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek\command /t REG_EXPAND_SZ /d "%script_dir%\seek.bat %%V -" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek0 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek0 /v MuiVerb /t REG_SZ /d "Specify calibs dir" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek0\command /t REG_EXPAND_SZ /d "%script_dir%\seek.bat %%V" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek1 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek1 /v MuiVerb /t REG_SZ /d "Search calibs here" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek1\command /t REG_EXPAND_SZ /d "%script_dir%\seek.bat %%V %%V" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek2 /v Icon /t REG_SZ /d %script_dir%\..\viewer.ico /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek2 /v MuiVerb /t REG_SZ /d "Search calibs at up level dir(&!)" /f
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\viewer.nseek2\command /t REG_EXPAND_SZ /d "%script_dir%\seek.bat %%V %%V\.." /f