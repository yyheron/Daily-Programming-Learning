@echo off 
set "script_dir=%~dp0"
set "name=%~nx1"
@REM pcbins
if "%name%" equ "pc_spot_hdr_0.bin" ( echo -file=%1 -type=4 -pack=80:10 -cs=tof && exit )
if "%name%" equ "pc_spot_hdr_1.bin" ( echo -file=%1 -type=4 -pack=80:10 -cs=tof && exit )
if "%name%" equ "pc_flood_0.bin" ( echo -file=%1 -type=4 -pack=240:90 -cs=tof && exit )
if "%name%" equ "pc_0_spot.bin" ( echo -file=%1 -type=5 -pack=-1:-1 -cs=tof && exit )
if "%name%" equ "pc_1_spot.bin" ( echo -file=%1 -type=5 -pack=-1:-1 -cs=tof && exit )
if "%name%" equ "pc_0_flood.bin" ( echo -file=%1 -type=5 -pack=-1:-1 -cs=tof && exit )
if "%name%" equ "pc_1_flood.bin" ( echo -file=%1 -type=5 -pack=-1:-1 -cs=tof && exit )
if "%name%" equ "pc_2_flood.bin" ( echo -file=%1 -type=5 -pack=-1:-1 -cs=tof && exit )
@REM slc calib bins
if "%name%" equ "Static.bin.tof_cali_decrypt" ( echo -file=%1 -type=1002 -cs=tof && exit )
if "%name%" equ "Static_down.bin.tof_cali_decrypt" ( echo -file=%1 -type=1002 -cs=tof && exit )
if "%name%" equ "Trans.bin.tof_cali_decrypt" ( echo -file=%1 -type=1002 -cs=tof && exit )
if "%name%" equ "Rotate.bin.tof_cali_decrypt" ( echo -file=%1 -type=1002 -cs=tof && exit )
if "%name%" equ "Static.bin" ( echo -file=%1 -type=1002 -cs=tof && exit )
if "%name%" equ "Static_down.bin" ( echo -file=%1 -type=1002 -cs=tof && exit )
if "%name%" equ "Trans.bin" ( echo -file=%1 -type=1002 -cs=tof && exit )
if "%name%" equ "Rotate.bin" ( echo -file=%1 -type=1002 -cs=tof && exit )
if "%name%" equ "Collect.bin" ( echo -file=%1 -type=1002 -cs=tof && exit )
@REM NAV bins
if "%name:~0,15%" equ "NAV_binId10.log" ( echo -file=%1 -type=1001 -cs=body && exit )
if "%name:~0,15%" equ "NAV_binId14.log" ( echo -file=%1 -type=1001 -cs=body && exit )
if "%name:~0,16%" equ "RRLDR_binId4.log" ( echo -file=%1 -type=106 -pack=32 -cs=body && exit )
@REM LIDAR bins
if "%name:~0,17%" equ "LIDAR_binId11.log" ( echo -file=%1 -type=1004 -cs=body && exit )
if "%name:~0,17%" equ "LIDAR_binId12.log" ( echo -file=%1 -type=1004 -cs=body && exit )

@REM ply, -pack=id:label:isSuperRes:width:height
if "%name:~-4%" equ ".ply" ( echo -file=%1 -type=50 -pack=0:3:1:240:96 -cs=tof && exit )
@REM ChaoFeng pc csv
if "%name:~0,11%" equ "RAW_ORIGIN_" ( if "%name:~-4%" equ ".csv" ( echo -file=%1 -type=58 -pack=120:48:3 -cs=tof && exit ) )
@REM StructLight pc csv
if "%name:~0,11%" equ "CloudPoint_" ( if "%name:~-4%" equ ".csv" ( echo -file=%1 -type=62 -cs=tof && exit ) )
@REM MT tof check
if "%name:~0,10%" equ "MT_TOF_Tof" ( if "%name:~-4%" equ ".dat" ( echo -file=%1 -type=52 && exit ) )
if "%name:~0,8%" equ "MT_ITOF_" ( if "%name:~-4%" equ ".dat" ( echo -file=%1 -type=52 && exit ) )
@REM AT_[width]x[height]_*.yuv, nv12
if "%name:~0,11%" equ "AT_800x600_" ( if "%name:~-4%" equ ".yuv" ( echo -file=%1 -type=301 && exit ) )
if "%name:~0,13%" equ "AT_1600x1200_" ( if "%name:~-4%" equ ".yuv" ( echo -file=%1 -type=301 && exit ) )
if "%name:~0,16%" equ "AT_RGB0_800x600_" ( if "%name:~-4%" equ ".yuv" ( echo -file=%1 -type=301 && exit ) )
if "%name:~0,16%" equ "AT_RGB1_800x600_" ( if "%name:~-4%" equ ".yuv" ( echo -file=%1 -type=301 && exit ) )
@REM armctl
if "%name%" equ "arm_space.bin" ( echo -file=%1 -type=1003 -cs=body && exit )
if "%name%" equ "arm_planner.bin" ( echo -file=%1 -type=1003 -cs=body && exit )
if "%name%" equ "armctl.log" ( echo -file=%1 -type=700 -cs=body && exit )
@REM armctl tof files
if "%name:~0,19%" equ "GEMPTY_CHECK_FIRST_" ( if "%name:~-4%" equ ".bin" ( echo -file=%1 -type=63 -cs=body && exit ) )
if "%name:~0,16%" equ "GEMPTY_CHECK_UP_" ( if "%name:~-4%" equ ".bin" ( echo -file=%1 -type=63 -cs=body && exit ) )
if "%name:~0,12%" equ "PLACE_FIRST_" ( if "%name:~-4%" equ ".bin" ( echo -file=%1 -type=63 -cs=body && exit ) )
@REM image.fs
if "%name:~0,2%" equ "L_" ( if "%name:~-9%" equ "_image.fs" ( echo -file=%1 -type=305 && exit ))
if "%name:~0,2%" equ "R_" ( if "%name:~-9%" equ "_image.fs" ( echo -file=%1 -type=305 && exit ))
@REM lidar mid360
if "%name:~0,13%" equ "lidar_mid360_" ( if "%name:~-4%" equ ".bin" ( if not "%name:~-7%" equ "imu.bin" ( echo -file=%1 -type=6 && exit ) ))
@REM MSC calib
if "%name%" equ "MSC.bin" ( echo -file=%1 -type=1006 && exit )