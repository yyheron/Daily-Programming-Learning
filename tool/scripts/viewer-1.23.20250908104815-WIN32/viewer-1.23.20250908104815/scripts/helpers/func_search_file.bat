@echo off
set "searchDir=%~1"
set "fileName=%~2"
for /r "%searchDir%" %%F in (%fileName%) do (
    if exist %%F ( echo %%F )
)
exit /b