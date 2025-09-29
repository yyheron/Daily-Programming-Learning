@echo off 
set "script_dir=%~dp0"
chcp 65001
setlocal enabledelayedexpansion
if "%1" neq "" ( 
    set "root=%1" 
) else (
    echo --------------------------------------------------
    set /p root=点云完整路径:
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

set "cmds="
for /f "tokens=*" %%B in ('call %script_dir%\helpers\func_get_file_cmds.bat %1') do set "cmds=%%B"
if "%cmds%" neq "" (
    echo !cmds! 
    %script_dir%\..\bin\viewer.exe !cmds! !calibstr!
)
if "%cmds%" equ "" ( echo 数据格式不支持 && pause )