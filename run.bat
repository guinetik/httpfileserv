@echo off
REM Run script for httpfileserv

REM Check if help parameter is requested
if "%~1"=="--help" goto :help
if "%~1"=="-h" goto :help
if "%~1"=="/?" goto :help

REM Check if the executable exists
if not exist bin\httpfileserv.exe (
    echo httpfileserv.exe not found. Please build the project first.
    echo Run build.bat to build the project.
    exit /b 1
)

REM Process directory path parameter
if "%~1"=="" (
    echo Using current directory as default...
    set "DIR_TO_SERVE=%CD%"
) else (
    set "DIR_TO_SERVE=%~1"
)

REM Check for port parameter
if "%~2"=="" (
    set "PORT=8080"
) else (
    set "PORT=%~2"
)

echo Starting HTTP File Server...
echo Serving directory: %DIR_TO_SERVE%
echo Port: %PORT%
echo.
echo Press Ctrl+C to stop the server
echo.

bin\httpfileserv.exe "%DIR_TO_SERVE%" %PORT%
goto :eof

:help
echo HTTP File Server Usage
echo =====================
echo.
echo Basic usage: 
echo   httpfileserv.exe ^<directory_path^> [port]
echo.
echo Examples:
echo   httpfileserv.exe "D:\Documents"         (Serves files from D:\Documents on default port 8080)
echo   httpfileserv.exe "D:\Documents" 9000    (Serves files from D:\Documents on port 9000)
echo.
echo Using run.bat:
echo   run.bat                         (Serves current directory on default port 8080)
echo   run.bat "D:\Documents"          (Serves files from D:\Documents on default port 8080)
echo   run.bat "D:\Documents" 9000     (Serves files from D:\Documents on port 9000)
echo   run.bat --help                  (Displays this help message)
echo.
exit /b 0
