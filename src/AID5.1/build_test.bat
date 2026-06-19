@echo off
echo ========================================
echo Building AID5.1 WYSIWYG Editor Test
echo ========================================

cd /d "%~dp0"

echo Cleaning previous build...
if exist "bin" rmdir /s /q "bin"
if exist "obj" rmdir /s /q "obj"

echo Building project...
msbuild AID4.1.sln /p:Configuration=Debug /p:Platform=x64 /m

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Executable location: bin\Debug\AID4.1.exe
    echo.
    echo You can now run the AID5.1 WYSIWYG Editor!
    echo.
) else (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    echo.
    echo Please check the error messages above.
    echo.
)

pause