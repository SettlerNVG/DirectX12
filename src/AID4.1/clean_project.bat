@echo off
echo Cleaning AID4.1 project...

REM Delete bin folder
if exist bin (
    echo Deleting bin folder...
    rmdir /s /q bin
)

REM Delete obj folder
if exist obj (
    echo Deleting obj folder...
    rmdir /s /q obj
)

REM Delete .vs folder
if exist .vs (
    echo Deleting .vs folder...
    rmdir /s /q .vs
)

REM Delete user files
if exist *.user (
    echo Deleting user files...
    del /q *.user
)

REM Delete log files
if exist engine_aid4.log (
    echo Deleting engine_aid4.log...
    del /q engine_aid4.log
)

echo.
echo Project cleaned successfully!
echo Please rebuild the project in Visual Studio.
echo.
pause
