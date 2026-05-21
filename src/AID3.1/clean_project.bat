@echo off
echo Cleaning AID3.1 project...

REM Delete build directories
if exist bin rmdir /s /q bin
if exist obj rmdir /s /q obj
if exist x64 rmdir /s /q x64
if exist Debug rmdir /s /q Debug
if exist Release rmdir /s /q Release

REM Delete Visual Studio files
if exist .vs rmdir /s /q .vs
del /q *.user 2>nul
del /q *.suo 2>nul
del /q *.sdf 2>nul
del /q *.opensdf 2>nul
del /q *.VC.db 2>nul
del /q *.VC.opendb 2>nul

REM Delete log files
del /q *.log 2>nul

echo Cleanup complete!
pause