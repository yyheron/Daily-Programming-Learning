chcp 65001
@echo off
setlocal enabledelayedexpansion
set /a groupid=0 
set "cmd="
REM 在以下区间设置你的组别，此行以上请勿更改

set cmd=!cmd! -group=!groupid! 
set cmd=!cmd! -file=C:\Users\me\Downloads\TEMP\2024-11-02\2_pack_log_tmp_data\1-1\Front\1\pc_0_flood.bin -type=5 -pack=-1:-1
set cmd=!cmd! -file=C:\Users\me\Downloads\TEMP\2024-11-02\2_pack_log_tmp_data\1-1\Back\1\pc_1_flood.bin -type=5 -pack=-1:-1
set cmd=!cmd! -rtfile=C:\Users\me\Downloads\TEMP\2024-11-02\2_pack_log_tmp_data\1-1\tof0.bin
set cmd=!cmd! -rtfile=C:\Users\me\Downloads\TEMP\2024-11-02\2_pack_log_tmp_data\1-1\tof1.bin
set /a groupid+=1 

REM 在以上区间设置你的组别，此行以下请勿更改
echo !cmd! 
set "script_dir=%~dp0"
if "!cmd!"  neq "" ( %script_dir%\..\bin\viewer.exe -uiwidth=400 !cmd! )
if "!cmd!" equ "" ( echo 没找到数据文件,请检查路径中是否有空格等特殊字符 && pause )