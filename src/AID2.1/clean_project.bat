@echo off
echo Cleaning AID2.1 project...

REM Delete build directories
if exist bin rmdir /s /q bin
if exist obj rmdir /s /q obj
if exist x64 rmdir /s /q x64
if exist Debug rmdir /s /q Debug
if exist Release rmdir /s /q Release
if exist .vs rmdir /s /q .vs

REM Delete Visual Studio user files
del /q *.suo 2>nul
del /q *.user 2>nul
del /q *.sdf 2>nul
del /q *.opensdf 2>nul
del /q *.db 2>nul
del /q *.opendb 2>nul
del /q *.VC.db 2>nul

REM Delete log files
del /q *.log 2>nul

echo Project cleaned successfully!
pause
