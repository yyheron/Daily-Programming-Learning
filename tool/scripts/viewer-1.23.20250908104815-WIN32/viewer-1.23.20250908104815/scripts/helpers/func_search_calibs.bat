@echo off && setlocal enabledelayedexpansion
set "script_dir=%~dp0"
set "tofcalib0="
set "tofcalib1="
set "tofcalib2="
set "rgbint="
set "rgbext="
set "calibstr="
if not "%1" equ "" (
    for /r %1 %%a in (*front_tof_SLCResult.bin) do ( set "tofcalib0=%%a" )
    if "!tofcalib0!" equ "" ( for /r %1 %%a in (*front_tof_LSCResult.bin) do ( set "tofcalib0=%%a" ) )
    if "!tofcalib0!" equ "" ( for /r %1 %%a in (*tof0.bin*) do ( set "tofcalib0=%%a" ) )
    if not "!tofcalib0!" equ "" ( set "calibstr=!calibstr! -rtfile=!tofcalib0!" )

    for /r %1 %%a in (*back_tof_SLCResult.bin) do ( set "tofcalib1=%%a" )
    if "!tofcalib1!" equ "" ( for /r %1 %%a in (*tof1.bin*) do ( set "tofcalib1=%%a" ) )
    if not "!tofcalib1!" equ "" ( set "calibstr=!calibstr! -rtfile=!tofcalib1!" )

    for /r %1 %%a in (*up_itof_SLCResult.bin) do ( set "tofcalib2=%%a" )
    if "!tofcalib2!" equ "" ( for /r %1 %%a in (*tof2.bin*) do ( set "tofcalib2=%%a" ) )
    if not "!tofcalib2!" equ "" ( set "calibstr=!calibstr! -rtfile=!tofcalib2!" )

    for /r %1 %%a in (*camera1_calib.bin) do ( set "rgbint=%%a" )
    if "!rgbint!" equ "" ( for /r %1 %%a in (*camera0_calib.bin) do ( set "rgbint=%%a" ) )
    if "!rgbint!" equ "" ( for /r %1 %%a in (*camera0_calib_v3.bin) do ( set "rgbint=%%a" ) )
    if "!rgbint!" equ "" ( for /r %1 %%a in (*camera1_calib_v3.bin) do ( set "rgbint=%%a" ) )
    if "!rgbint!" equ "" ( for /r %1 %%a in (*camera0_calib_fisheye.bin) do ( set "rgbint=%%a" ) )
    if "!rgbint!" equ "" ( for /r %1 %%a in (*camera1_calib_fisheye.bin) do ( set "rgbint=%%a" ) )
    if "!rgbint!" equ "" ( for /r %1 %%a in (*rgb_intrinsic.param) do ( set "rgbint=%%a" ) )
    if "!rgbint!" equ "" ( for /r %1 %%a in (*SN*_dualcamera_calibration_input.json) do ( set "rgbint=%%a" ) )
    if not "!rgbint!" equ "" ( set "calibstr=!calibstr! -rgbint=!rgbint!" )

    for /r %1 %%a in (*camera1_depth.bin) do ( set "rgbext=%%a" )
    if "!rgbext!" equ "" ( for /r %1 %%a in (*camera0_depth.bin) do ( set "rgbext=%%a" ) )
    if "!rgbext!" equ "" ( for /r %1 %%a in (*rgb_extrinsic.param) do ( set "rgbext=%%a" ) )
    if "!rgbext!" equ "" ( for /r %1 %%a in (*_rgb.bin) do ( set "rgbext=%%a" ) )
    if not "!rgbext!" equ "" ( set "calibstr=!calibstr! -rgbext=!rgbext!" )
)
echo !calibstr!