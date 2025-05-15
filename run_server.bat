@echo off
REM Run script for httpfileserv

if not exist bin\httpfileserv.exe (
    echo httpfileserv.exe not found. Please build the project first.
    echo Run build.bat to build the project.
    exit /b 1
)

if "%~1"=="" (
    echo Using current directory as default...
    set "DIR_TO_SERVE=%CD%"
) else (
    set "DIR_TO_SERVE=%~1"
)

echo Starting HTTP File Server...
echo Serving directory: %DIR_TO_SERVE%
echo Port: 8080
echo.
echo Press Ctrl+C to stop the server
echo.

bin\httpfileserv.exe "%DIR_TO_SERVE%"
