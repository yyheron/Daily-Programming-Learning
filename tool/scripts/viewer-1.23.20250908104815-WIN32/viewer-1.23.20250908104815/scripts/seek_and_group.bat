@echo off 
set "script_dir=%~dp0"
chcp 65001
setlocal enabledelayedexpansion
if "%1" neq "" ( 
    set "root=%1" 
) else (
    echo --------------------------------------------------
    set /p root=点云搜索路径（递归搜索，必选）:
)
if "!root!" equ "" (
    echo 您必须指定点云搜索路径！！！
    exit 0
)
if "%2" neq "" ( 
    set "calibs=%2" 
) else (
    echo --------------------------------------------------
    set /p calibs=标定搜索路径（递归搜索，可直接回车跳过）:
)
for /f "tokens=*" %%B in ('call %script_dir%\helpers\func_search_calibs.bat %calibs%') do ( set calibstr=%%B )
echo --------------------------------------------------
set /a groupid=0 
set "cmds="
for /d /r %root% %%a in (*) do ( 
    set "ccmd="
    for %%b in (%%a\*) do ( for /f "tokens=*" %%c in ('call %script_dir%\helpers\func_get_file_cmds.bat %%b') do ( if "%%c" neq "" ( set "ccmd=!ccmd! %%c" && echo %%c ) ) )
    if "!ccmd!" neq "" ( set "cmds=!cmds! -group=!groupid! !ccmd! !calibstr!" && echo ------------- Above is group !groupid! ------------- && set /a groupid+=1 )
)
set "ccmd="
for %%a in (%root%\*) do ( for /f "tokens=*" %%c in ('call %script_dir%\helpers\func_get_file_cmds.bat %%a') do ( if "%%c" neq "" ( set "ccmd=!ccmd! %%c" && echo %%c ) ) )
if "!ccmd!" neq "" ( set "cmds=!cmds! -group=!groupid! !ccmd! !calibstr!" && echo ------------- Above is group !groupid! ------------- && set /a groupid+=1 )

if "!cmds!" neq "" ( %script_dir%\..\bin\viewer.exe -uiwidth=400 !cmds! )
if "!cmds!" equ "" ( echo 没找到数据文件,请检查路径中是否有空格等特殊字符 && pause )